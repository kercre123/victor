prebuilt_jar(
  name = 'unity_classes',
  binary_jar = 'project/android/cozmojava/lib/unity-classes.jar'
)

android_library(
  name = 'cozmojava_lib',
  srcs = glob(['project/android/cozmojava/src/**/*.java']),
  manifest = 'project/android/cozmojava/src/main/AndroidManifest.xml',
  source = '1.6',
  target = '1.6',
  deps = [':unity_classes',
          '//lib/util/android:util',
          '//lib/das-client/android/DASJavaLib:DASJavaLib'],
  visibility = ['PUBLIC'],
)

android_aar(
  name = 'cozmojava_with_unity',
  manifest_skeleton = 'project/android/AndroidManifest.xml',
  deps = [
    ':cozmojava_lib',
    '//lib/util/android:android-support-v4-23-aar',
    '//lib/util/android:play-services-location',
    '//lib/util/android:play-services-base'
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
