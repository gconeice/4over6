package com.example.a4over6;

import android.content.Intent;
import android.content.SharedPreferences;
import android.os.ParcelFileDescriptor;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;

import java.io.BufferedOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.InetAddress;
import java.util.Arrays;
import java.util.Timer;
import java.util.TimerTask;

import static java.lang.Thread.sleep;

public class MyVpnService extends android.net.VpnService {

    static {
//        System.loadLibrary("libc++_shared.so");
        System.loadLibrary("hellojni");
    }
    public native int vpn_entry(String hostName, int port, int commandReadFd, int responseWriteFd);

    private static final String TAG = "VPN Service";

    public static String BROADCAST = "VPN_BOARDCAST";

    public static final String START = "VPN_START";
    public static final String STOP = "VPN_STOP";

    static int COMMAND_EXIT  = 1;
    static int COMMAND_FETCH_CONFIG = 2;
    static int COMMAND_SET_TUN = 3;
    static int COMMAND_FETCH_STATE = 4;

    private static MyVpnService instance = null;

    private ParcelFileDescriptor commandWriteFd;
    private ParcelFileDescriptor commandReadFd;
    private ParcelFileDescriptor responseWriteFd;
    private ParcelFileDescriptor responseReadFd;

    private DataInputStream responseReadStream;
    private DataOutputStream commandWriteStream;

    private Thread vpnThread = null;

    private ParcelFileDescriptor vpnInterface = null;
    private Timer timer = null;

    private String virtual_ipv4_addr;
    private long in_bytes;
    private long out_bytes;
    private long in_packets;
    private long out_packets;
    private long in_speed_bytes;
    private long out_speed_bytes;
    private long running_time;

    private void broadcastState() {
        Intent intent = new Intent(BROADCAST);
        intent.putExtra("running_state", (vpnThread != null && vpnThread.isAlive()) ? 1 : 0);
        intent.putExtra("virtual_ipv4_addr", virtual_ipv4_addr);
        intent.putExtra("in_bytes", in_bytes);
        intent.putExtra("out_bytes", out_bytes);
        intent.putExtra("in_packets", in_packets);
        intent.putExtra("out_packets", out_packets);
        intent.putExtra("in_speed_bytes", in_speed_bytes);
        intent.putExtra("out_speed_bytes", out_speed_bytes);
        intent.putExtra("running_time", running_time);

        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    private byte[] readResponse(int length) throws IOException {
        byte[] data = new byte[length];
        int offset = 0;
        while(offset < length) {
            int read = -9999;
//            for (; read != -1;) {
                read = responseReadStream.read(data, offset, length - offset);

//            }
            if (read < 0) throw new IOException("read fail");
            offset += read;
        }
        return data;
    }

    private int readInt(DataInputStream s) throws IOException {
        int a = s.read();
        int b = s.read();
        int c = s.read();
        int d = s.read();
        return ((d*256+c)*256+b)*256+a;
    }
    private long readLong(DataInputStream s) throws IOException {
        long a = (long)readInt(s);
        long b = (long)readInt(s);
        return (b<<32)+a;
    }
    private void writeInt(DataOutputStream s, int data) throws IOException {
        int a = data%256; data /= 256;
        int b = data%256; data /= 256;
        int c = data%256; data /= 256;
        int d = data%256; data /= 256;
        s.write(a);s.write(b);s.write(c);s.write(d);
    }

    @Override
    public int onStartCommand(final Intent intent, int flags, int startId) {

        new Thread(new Runnable() {
            @Override
            public void run() {
                if (intent.getAction().equals(START)) {
                    Log.d("onStartCommand", "starting");
                    startVPN(intent.getStringExtra("hostname"), intent.getIntExtra("port", 0));
                    MyVpnService.instance = MyVpnService.this;
                } else {
                    Log.d("onStartCommand", "stopping");
                    if (MyVpnService.instance != null) MyVpnService.instance.stopVPN();
                    MyVpnService.instance = null;
                }
                broadcastState();
            }
        }).start();

        return START_REDELIVER_INTENT;
    }

    private int startVPN(String hostname, int port) {
        Log.d("onStartCommand", hostname);
        if (vpnThread != null) {
            Log.d("onStartCommand", "vpn thread not null");
        }
//        stopVPN();

        virtual_ipv4_addr = null;
        in_bytes = 0;
        out_bytes = 0;
        in_packets = 0;
        out_packets = 0;
        in_speed_bytes = 0;
        out_speed_bytes = 0;
        running_time = 0;

        try {
            ParcelFileDescriptor[] pipeFds = ParcelFileDescriptor.createPipe();
            commandWriteFd = pipeFds[1];
            commandReadFd = pipeFds[0];
            pipeFds = ParcelFileDescriptor.createPipe();
            responseWriteFd = pipeFds[1];
            responseReadFd = pipeFds[0];
        } catch (IOException e) {
            e.printStackTrace();
            return -1;
        }

        final String f_hostname = hostname;
        final int f_port = port;
        final int commandRead = commandReadFd.getFd();
        final int responseWrite = responseWriteFd.getFd();

        vpnThread = new Thread(new Runnable() {
            @Override
            public void run() {
                final int ret = vpn_entry(f_hostname, f_port, commandRead, responseWrite);
            }
        });
        vpnThread.start();

        commandWriteStream = new DataOutputStream(new FileOutputStream(commandWriteFd.getFileDescriptor()));
        responseReadStream = new DataInputStream(new FileInputStream(responseReadFd.getFileDescriptor()));

        // 初始化配置
        try {
            commandWriteStream.writeByte(COMMAND_FETCH_CONFIG);
//            sleep(1000);
            byte[] data = readResponse(20);
            InetAddress address = InetAddress.getByAddress(Arrays.copyOfRange(data, 0, 4));
            InetAddress mask = InetAddress.getByAddress(Arrays.copyOfRange(data, 4, 8));
            InetAddress dns1 = InetAddress.getByAddress(Arrays.copyOfRange(data, 8, 12));
            InetAddress dns2 = InetAddress.getByAddress(Arrays.copyOfRange(data, 12, 16));
            InetAddress dns3 = InetAddress.getByAddress(Arrays.copyOfRange(data, 16, 20));
            int socketFd = readInt(responseReadStream);

            vpnInterface = new Builder()
                    .addAddress(address, 24)
                    .addDnsServer(dns1)
                    .addDnsServer(dns2)
                    .addDnsServer(dns3)
                    .addRoute("0.0.0.0", 0)
                    .setSession("4over6")
                    .establish();
            commandWriteStream.writeByte(COMMAND_SET_TUN);
            writeInt(commandWriteStream, vpnInterface.getFd());
            protect(socketFd);

            virtual_ipv4_addr = address.getHostAddress();
        } catch (IOException e) {
            e.printStackTrace();
            return -1;
        }

        TimerTask timer_task = new TimerTask() {
            public void run() {
                running_time ++;

                if (!vpnThread.isAlive()) {
                    Log.d("onStartCommand", "call vpn stop");
                    stopVPN();
                } else {
                    try {
                        commandWriteStream.writeByte(COMMAND_FETCH_STATE);
                        long old_in_bytes = in_bytes;
                        long old_out_bytes = out_bytes;
                        in_bytes = readLong(responseReadStream);
                        out_bytes = readLong(responseReadStream);
                        in_packets = readLong(responseReadStream);
                        out_packets = readLong(responseReadStream);
                        in_speed_bytes = in_bytes - old_in_bytes;
                        out_speed_bytes = out_bytes - old_out_bytes;
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
                broadcastState();
            }
        };

        timer = new Timer();
        timer.scheduleAtFixedRate(timer_task, 1000, 1000);

        return 0;
    }

    private void stopVPN() {


        if (timer != null) {
            timer.cancel();
        }
        timer = null;

        if (vpnThread != null && vpnThread.isAlive()) {
            Log.d("onStartCommand", "vpn stop called");
            try {
                commandWriteStream.writeByte(COMMAND_EXIT);
                vpnThread.join(10000);
                commandWriteStream.close();
                responseReadStream.close();
            } catch (InterruptedException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        vpnThread = null;

        if (vpnInterface != null) {
            try {
                vpnInterface.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        vpnInterface = null;

        stopSelf();
    }

    @Override
    public void onDestroy() {
        stopVPN();
    }
}

