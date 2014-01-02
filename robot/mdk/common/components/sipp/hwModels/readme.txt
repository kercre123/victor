External files are linked to this project through svn:externals.
From GUI: right-click on "hwModels"->TortoiseSVN->Properties and edit "svn:externals" property.
Tools paths is changling every few months, so one might need to update the svn properties.

Unfortunately latest SVN doesn't allow single-file externals from different repo,
so I need to export entire directories SCR/INC from tools trunk. But the only files
needed are: moviFloat32.h/c,  moviThreadsUtils.h/c

(strangely, the older 1.7.6.22632 TortoiseSVN allowed for single files from different repository,
but it also generated some error message).

