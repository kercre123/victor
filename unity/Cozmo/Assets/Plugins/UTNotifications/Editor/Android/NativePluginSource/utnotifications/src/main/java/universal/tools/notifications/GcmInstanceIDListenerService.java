package universal.tools.notifications;

import android.app.IntentService;
import android.content.Intent;
import android.util.Log;

import com.google.android.gms.gcm.GoogleCloudMessaging;
import com.google.android.gms.iid.InstanceID;

import java.io.IOException;

public class GcmInstanceIDListenerService extends IntentService {
    public static final String SENDER_ID = "SENDER_ID";
    public static final String UNREGISTER = "__UNREGISTER";

    public GcmInstanceIDListenerService() {
        super("GcmInstanceIDListenerService");
    }

    @Override
    public void onHandleIntent(Intent intent) {
        if (intent == null) {
            return;
        }

        String senderId = intent.getStringExtra(SENDER_ID);

        if (senderId != null && !senderId.isEmpty()) {
            InstanceID instanceID = InstanceID.getInstance(this);

            if (senderId.equals(UNREGISTER)) {
                try {
                    instanceID.deleteInstanceID();
                } catch (IOException e) {
                    Log.e("UTNotifications", "Unable to unregister FCM: " + e.getMessage());
                }
            } else {
                try {
                    String token = instanceID.getToken(senderId, GoogleCloudMessaging.INSTANCE_ID_SCOPE, null);
                    Manager.onRegistered(FCMProvider.Name, token);
                } catch (IOException e) {
                    Log.e("UTNotifications", "Unable to register FCM: " + e.getMessage());
                }
            }
        }
    }
}
