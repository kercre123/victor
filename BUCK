android_prebuilt_aar(
  name = 'android-support',
  aar = 'project/android/cozmojava/lib/support-v4-23.4.0.aar',
  visibility = ['PUBLIC'],
)

android_library(
  name = 'cozmojava_lib',
  srcs = glob(['project/android/cozmojava/src/**/*.java']),
  manifest = 'project/android/cozmojava/src/main/AndroidManifest.xml',
  source = '1.6',
  target = '1.6',
  deps = ['//lib/util/android:util',
          '//lib/das-client/android/DASJavaLib:DASJavaLib',
          ':android-support'],
  visibility = ['PUBLIC'],
)

android_aar(
  name = 'cozmojava',
  manifest_skeleton = 'project/android/AndroidManifest.xml',
  deps = [
    ':cozmojava_lib',
    ':android-support',
  ],
  visibility = ['PUBLIC'],
)
