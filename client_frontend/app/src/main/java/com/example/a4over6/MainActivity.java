package com.example.a4over6;

import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.Enumeration;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
//        TextView textView = new TextView(this);
//        textView.setText(StringFromJNI());
//        setContentView(textView);


    }

    public void onStartVPNClicked(View view) {
        Intent intent = MyVpnService.prepare(this);
        if (intent != null) {
            startActivityForResult(intent, 0);
        } else {
            onActivityResult(0, RESULT_OK, null);
        }
        TextView text = findViewById(R.id.textView);
//        text.setText(StringFromJNI());
        text.setText(getIPv6Address(this));
    }

    protected void onActivityResult(int request, int result, Intent data) {
        if (result == RESULT_OK) {
            Intent intent = new Intent(this, MyVpnService.class);
            startService(intent);
        }
    }

    public native String StringFromJNI();
    static {
        System.loadLibrary("hellojni");
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

    private static boolean isWIFIConnected(Context context)
    {
        // Context.CONNECTIVITY_SERVICE).
        ConnectivityManager manager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo networkInfo = manager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
        if (networkInfo != null && networkInfo.isConnected())
        {
            // Log.e("Net", "Wifi 已连接");
            return true;
        }
        // Log.e("Net", "无Wifi连接");
        return false;
    }
}
