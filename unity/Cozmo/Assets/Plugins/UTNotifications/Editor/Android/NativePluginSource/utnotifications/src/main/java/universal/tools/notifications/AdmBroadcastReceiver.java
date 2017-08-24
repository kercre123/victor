package universal.tools.notifications;

import com.amazon.device.messaging.ADMMessageReceiver;

public class AdmBroadcastReceiver extends ADMMessageReceiver {
    public AdmBroadcastReceiver() {
        super(AdmIntentService.class);
    }
}