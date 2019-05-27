package com.example.a4over6;

import android.annotation.SuppressLint;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.VpnService;
import android.os.Handler;
import android.os.Message;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.format.Formatter;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;

import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.Enumeration;

import static java.lang.Thread.sleep;

public class MainActivity extends AppCompatActivity {

    public boolean isVPNStart = false;

    private int running_state = 0;
    private String virtual_ipv4_addr = "";
    private long in_bytes = 0;
    private long out_bytes = 0;
    private long in_packets = 0;
    private long out_packets = 0;
    private long in_speed_bytes = 0;
    private long out_speed_bytes = 0;
    private long running_time = 0;

    private BroadcastReceiver vpn_state_receiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(MyVpnService.BROADCAST)) {
                MainActivity.this.running_state = intent.getIntExtra("running_state", 0);
                MainActivity.this.virtual_ipv4_addr = intent.getStringExtra("virtual_ipv4_addr");
                MainActivity.this.in_bytes = intent.getLongExtra("in_bytes", 0);
                MainActivity.this.out_bytes = intent.getLongExtra("out_bytes", 0);
                MainActivity.this.in_packets = intent.getLongExtra("in_packets", 0);
                MainActivity.this.out_packets = intent.getLongExtra("out_packets", 0);
                MainActivity.this.in_speed_bytes = intent.getLongExtra("in_speed_bytes", 0);
                MainActivity.this.out_speed_bytes = intent.getLongExtra("out_speed_bytes", 0);
                MainActivity.this.running_time = intent.getLongExtra("running_time", 0);

//                findViewById(R.id.connect_button).setEnabled(true);
//                findViewById(R.id.restart_button).setEnabled(true);
            }
            update();
        }
    };

    private void update() {
        TextView info = (TextView) findViewById(R.id.textView6);
        info.setText(
//                " local IPv4 : " + my_ipv4_addr + "\n" +
                        " local IPv6 : " + getIPv6Address(this) + "\n" +
                        " remote IPv6 : " + ((EditText)findViewById(R.id.editText)).getText().toString() + "\n" +
                        " remote port : " + Integer.parseInt(((EditText)findViewById(R.id.editText2)).getText().toString()) + "\n"+
                        " virtual IPv4 : " + virtual_ipv4_addr + "\n" +
                        " " + (running_state == 0 ? "Disconnected" : "Connected") + "\n" +
                        " Running Time : " + int2time(running_time) + "\n" +
                        " -----------------------\n" +
                        " Up : " + Formatter.formatShortFileSize(MainActivity.this, out_bytes) + "\n" +
                        " Up Speed : " + Formatter.formatShortFileSize(MainActivity.this, out_speed_bytes) + "/s\n" +
                        " Up Packets : " + out_packets + "\n" +
                        " -----------------------\n" +
                        " Down : " + Formatter.formatShortFileSize(MainActivity.this, in_bytes) + "\n" +
                        " Down Speed : " + Formatter.formatShortFileSize(MainActivity.this, in_speed_bytes) + "/s\n" +
                        " Down Packets : " + in_packets + "\n"
        );
    }

    private String int2time(long duration) {
        long second = duration % 60;
        long minute = (duration / 60) % 60;
        long hour = duration / 3600;
        return String.format("%d:%02d:%02d", hour, minute, second);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        update();
//        TextView textView = new TextView(this);
//        textView.setText(StringFromJNI());
//        setContentView(textView);
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(MyVpnService.BROADCAST);
        LocalBroadcastManager.getInstance(this).registerReceiver(vpn_state_receiver, intentFilter);

    }

    public void onStartVPNClicked(View view) {
        Intent intent = VpnService.prepare(MainActivity.this);
        if (intent != null) {
            startActivityForResult(intent, 0);
        } else {
            onActivityResult(0, RESULT_OK, null);
        }
        isVPNStart = true;
        update();

//        TextView text = findViewById(R.id.textView);
////        text.setText(StringFromJNI());
//        text.setText(getIPv6Address(this));
    }

    public void onStopVPNClicked(View view) {
        isVPNStart = false;
        Intent intent = new Intent(this, MyVpnService.class);
        startService(intent.setAction(MyVpnService.STOP));
    }

    protected void onActivityResult(int request, int result, Intent data) {
        if (result == RESULT_OK) {
            Intent intent = new Intent(this, MyVpnService.class);
            intent.putExtra("hostname", ((EditText)findViewById(R.id.editText)).getText().toString());
            intent.putExtra("port", Integer.parseInt(((EditText)findViewById(R.id.editText2)).getText().toString()));
            startService(intent.setAction(MyVpnService.START));
        }
    }

    static String getIPv6Address(Context context) {
        if(! isWIFIConnected(context)) {
            return null;
        }
        try {
            final Enumeration<NetworkInterface> e = NetworkInterface.getNetworkInterfaces();
            while (e.hasMoreElements()) {
                final NetworkInterface networkInterface = e.nextElement();
                for (Enumeration<InetAddress> enumAddress = networkInterface.getInetAddresses();
                     enumAddress.hasMoreElements(); ) {
                    InetAddress inetAddress = enumAddress.nextElement();
                    if (!inetAddress.isLoopbackAddress() && !inetAddress.isLinkLocalAddress()) {
                        return inetAddress.getHostAddress();
                    }
                }
            }
        } catch (SocketException e) {
            Log.e("NET", "无法获取IPV6地址");
        }
        return null;
    }

    private static boolean isWIFIConnected(Context context) {
        // Context.CONNECTIVITY_SERVICE).
        ConnectivityManager manager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo networkInfo = manager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
        if (networkInfo != null && networkInfo.isConnected()) {
            // Log.e("Net", "Wifi 已连接");
            return true;
        }
        // Log.e("Net", "无Wifi连接");
        return false;
    }

}
