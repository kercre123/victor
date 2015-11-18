
Dim unityPath 
unityPath = CreateObject("Scripting.FileSystemObject").GetAbsolutePathName("..\..\unity\Cozmo\Assets\Scripts\")
' msgbox unityPath

Dim cladZipFilePath 
cladZipFilePath = CreateObject("Scripting.FileSystemObject").GetAbsolutePathName("..\..\generated\csharpGeneratedClad.zip")
' msgbox cladZipFilePath

'If the extraction location does not exist create it.
Set fso = CreateObject("Scripting.FileSystemObject")
If NOT fso.FolderExists(unityPath) Then
   fso.CreateFolder(unityPath)
End If

'Extract the contants of the zip file.
Dim objShell
set objShell = CreateObject("Shell.Application")
set FilesInZip=objShell.NameSpace(cladZipFilePath).items()
objShell.NameSpace(unityPath).CopyHere FilesInZip , 16 
Set fso = Nothing
Set objShell = Nothing


' compile solution
const DontWaitUntilFinished = false, ShowWindow = 1, DontShowWindow = 0, WaitUntilFinished = true
Set oShell = WScript.CreateObject ("WScript.Shell")
oShell.run "cmd /c ""C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\IDE\devenv.com"" ..\..\animation-tool\AnimationTool.sln /Build Release" , DontShowWindow , WaitUntilFinished
Set oShell = Nothing


Set oShell = WScript.CreateObject ("WScript.Shell")
oShell.run "cmd /c ""C:\Program Files\7-zip\7z.exe"" a ..\..\animationTool.zip ..\..\animation-tool\AnimationTool\bin\Release\*.*" , DontShowWindow , WaitUntilFinished
Set oShell = Nothing

