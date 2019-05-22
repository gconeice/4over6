package com.example.a4over6;

import android.util.Log;

public class MyVpnService extends android.net.VpnService implements Runnable {

    private static final String TAG = "VPN Service";

    @Override
    public synchronized void run() {

        // mHandler.sendEmptyMessage(R.string.connecting);

        try {
            Log.e(TAG, "Starting");
//            configure();


            Log.e(TAG, "VPN配置已完成");
            // run_vpn();
            // flush();
            Log.e(TAG, "end?");

        } catch (Exception e) {
            Log.e(TAG, "Got " + e.toString());
        } finally {

        }
    }
}
