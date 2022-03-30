# HACK 2.0 SCREEN

This gets a quick and dirty version of the screen working. It also fixes the problem with cloud 850 errors because we have a bad vic-cloud and vic-engine.

1. Start with the latest 2.0 release availatble at the time. Currently 6060d. http://assets.digitaldreamlabs.com/vic/ufxTn3XGcVNK2YrF/v20/dev/vicos-2.0.0.6060d.ota

2. Build like normal

3. Run `./patch_cloud_bins.sh` to include the good versions of `vic-cloud` and `vic-gateway`

3. Deploy like normal.

Grant did experience some problems with certificates which went away. If you see that debug with him.
