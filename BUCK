prebuilt_jar(
  name = 'unity_classes',
  binary_jar = 'project/android/cozmojava/lib/unity-classes.jar'
)


android_prebuilt_aar(
  name = 'play-services-gcm',
  aar = 'project/android/cozmojava/lib/play-services-gcm-10.2.4.aar',
  visibility = ['PUBLIC'],
)

android_prebuilt_aar(
  name = 'play-services-iid',
  aar = 'project/android/cozmojava/lib/play-services-iid-10.2.4.aar',
  visibility = ['PUBLIC'],
)

android_prebuilt_aar(
  name = 'play-services-tasks',
  aar = 'project/android/cozmojava/lib/play-services-tasks-10.2.4.aar',
  visibility = ['PUBLIC'],
)

android_prebuilt_aar(
  name = 'support-core-ui',
  aar = 'project/android/cozmojava/lib/support-core-ui-26.0.0-alpha1.aar',
  visibility = ['PUBLIC'],
)

android_prebuilt_aar(
  name = 'support-core-utils',
  aar = 'project/android/cozmojava/lib/support-core-utils-26.0.0-alpha1.aar',
  visibility = ['PUBLIC'],
)

android_prebuilt_aar(
  name = 'support-fragment',
  aar = 'project/android/cozmojava/lib/support-fragment-26.0.0-alpha1.aar',
  visibility = ['PUBLIC'],
)

android_prebuilt_aar(
  name = 'support-media-compat',
  aar = 'project/android/cozmojava/lib/support-media-compat-26.0.0-alpha1.aar',
  visibility = ['PUBLIC'],
)


# Acapela Text-To-Speech SDK
prebuilt_jar(
  name = 'acattsandroid-sdk-library',
  binary_jar = 'EXTERNALS/anki-thirdparty/acapela/AcapelaTTS_for_Android_V1.612/sdk/acattsandroid-sdk-library.jar'
)

android_library(
  name = 'cozmojava_lib',
  srcs = glob(['project/android/cozmojava/src/**/*.java']),
  manifest = 'project/android/cozmojava/src/main/AndroidManifest.xml',
  source = '1.6',
  target = '1.6',
  deps = [':unity_classes',
          ':play-services-gcm',
          ':play-services-iid',
          ':play-services-tasks',
          ':support-core-ui',
          ':support-core-utils',
          ':support-fragment',
          ':support-media-compat',
          ':acattsandroid-sdk-library',
          '//lib/util/android:util',
          '//lib/crash-reporting-android/HockeyAppAndroid:hockey-app',
          '//lib/das-client/android/DASJavaLib:DASJavaLib'],
  visibility = ['PUBLIC'],
)

android_aar(
  name = 'cozmojava_with_unity',
  manifest_skeleton = 'project/android/AndroidManifest.xml',
  deps = [
    ':cozmojava_lib',
    '//lib/util/android:play-services-location',
    '//lib/util/android:play-services-base',
    '//lib/util/android:play-services-basement',
    '//lib/util/android:support-compat',
    '//lib/util/android:support-v4'
  ]
)

# Buck's build of the cozmojava library will include Unity's Java classes, but
# Unity will try to add them again later in the build process when it builds
# our final package, causing an error. This script removes Unity's Java classes
# from the built aar so there won't be duplicates.
export_file(
  name = 'unity_surgery_script',
  src = 'tools/android/aarUnityRemoval.sh'
)

genrule(
  name = 'cozmojava',
  bash = '$(location :unity_surgery_script) $(location :cozmojava_with_unity) $OUT',
  out = 'cozmojava.aar'
)

prebuilt_native_library(
  name = 'native_libs_for_standalone',
  native_libs = 'build/android/libs',
  visibility = [
    '//standalone-apk/java/com/anki/cozmoengine:lib',
  ],
)

android_binary(
  name = 'cozmoengine_standalone_app',
  manifest = 'standalone-apk/AndroidManifest.xml',
  keystore = '//standalone-apk/keystores:debug',
  deps = [
    '//standalone-apk/java/com/anki/cozmoengine:lib',
  ],
  visibility = ['PUBLIC'],
)
