package universal.tools.notifications;

import android.content.Intent;
import android.util.Log;

import com.amazon.device.messaging.ADMMessageHandlerBase;

public class AdmIntentService extends ADMMessageHandlerBase {
//public
    public AdmIntentService() {
        super("AdmIntentService");
    }

//protected
    @Override
    protected void onRegistered(final String newRegistrationId) {
        Manager.onRegistered(ADMProvider.Name, newRegistrationId);
    }

    @Override
    protected void onUnregistered(final String registrationId) {}

    @Override
    protected void onRegistrationError(final String errorId) {
        Log.e("UTNotifications", "Unable to register ADM ID: error " + errorId);
    }

    @Override
    protected void onMessage(final Intent intent) {
        Manager.postPushNotification(getApplicationContext(), intent.getExtras());
    }
}
