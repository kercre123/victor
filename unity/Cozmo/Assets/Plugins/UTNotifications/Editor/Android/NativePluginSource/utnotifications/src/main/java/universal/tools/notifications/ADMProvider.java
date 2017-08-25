package universal.tools.notifications;

import android.content.Context;
import android.util.Log;

import com.amazon.device.messaging.ADM;

public class ADMProvider implements IPushNotificationsProvider {
    public static final String Name = "ADM";

    public static boolean isAvailable() {
        try {
            Class.forName("com.amazon.device.messaging.ADM");
            return true;
        } catch (ClassNotFoundException e) {
            return false;
        }
    }

    public ADMProvider(Context context) {
        this.context = context;
    }

    @Override
    public void enable() {
        try {
            final ADM adm = new ADM(context);

            String registrationId = adm.getRegistrationId();
            if (registrationId == null) {
                adm.startRegister();
            } else {
                Manager.onRegistered(Name, registrationId);
            }
        } catch (SecurityException ex) {
            Log.e("UTNotifications", "Unable to register ADM: " + ex.getMessage());
        }
    }

    @Override
    public void disable() {
        final ADM adm = new ADM(context);
        adm.startUnregister();
    }

    private final Context context;
}
