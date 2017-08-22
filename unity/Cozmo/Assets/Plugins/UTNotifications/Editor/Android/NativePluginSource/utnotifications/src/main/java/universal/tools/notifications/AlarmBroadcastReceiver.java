package universal.tools.notifications;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

public class AlarmBroadcastReceiver extends BroadcastReceiver {
    public void onReceive(Context context, Intent intent) {
        int id = intent.getIntExtra(Manager.KEY_ID, 1);
        boolean repeated = intent.getBooleanExtra(Manager.KEY_REPEATED, false);
        Bundle userData = intent.getParcelableExtra(Manager.KEY_USER_DATA);

        Manager.postNotification(context, userData.getString(Manager.KEY_TITLE), userData.getString(Manager.KEY_TEXT), id, userData, userData.getString(Manager.KEY_NOTIFICATION_PROFILE), userData.getInt(Manager.KEY_BADGE_NUMBER, -1), repeated);
    }
}