package com.brobwind.rpi.tool;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Parcel;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;


public class RPiToolReceiver extends BroadcastReceiver {
    private static final String TAG = "RPi Tool";

    private static final String SURFACE_FLINGER_SERVICE_KEY = "SurfaceFlinger";

    private static final int SURFACE_FLINGER_DISABLE_OVERLAYS_CODE = 1008;
    private static final String SURFACE_COMPOSER_INTERFACE_KEY = "android.ui.ISurfaceComposer";

    @Override
    public void onReceive(Context context, Intent intent) {
        IBinder sf = ServiceManager.getService(SURFACE_FLINGER_SERVICE_KEY);
        if (sf == null) {
            return;
        }

        Log.v(TAG, "Try to disable hardware overlay on surfaceflinger ...");

        try {
            final Parcel data = Parcel.obtain();
            data.writeInterfaceToken(SURFACE_COMPOSER_INTERFACE_KEY);
            final int disableOverlays = 1;
            data.writeInt(disableOverlays);
            sf.transact(SURFACE_FLINGER_DISABLE_OVERLAYS_CODE, data,
                    null /* reply */, 0 /* flags */);
            data.recycle();
        } catch (RemoteException ex) {
            ex.printStackTrace();
        }
    }
}
