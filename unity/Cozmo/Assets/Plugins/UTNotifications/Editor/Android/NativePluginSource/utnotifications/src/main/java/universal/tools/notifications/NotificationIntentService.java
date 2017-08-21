package universal.tools.notifications;

import android.app.IntentService;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;

import com.unity3d.player.UnityPlayer;

public class NotificationIntentService extends IntentService {
//public
    public NotificationIntentService() {
        super("NotificationIntentService");
    }

//protected
    @Override
    protected void onHandleIntent(Intent intent) {
        if (intent == null) {
            return;
        }

        PackageManager packageManager = this.getApplicationContext().getPackageManager();
        Intent mainActivityIntent = packageManager.getLaunchIntentForPackage(this.getApplicationContext().getPackageName());
        mainActivityIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mainActivityIntent.addCategory(Intent.CATEGORY_LAUNCHER);
        Manager.setIntent(intent);
        startActivity(mainActivityIntent);
    }
}
