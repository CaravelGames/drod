# Preface

This is an old document assembled by [silver](http://forum.caravelgames.com/member.php?Action=viewprofile&username=silver) for building TCB. In most part it's accurate but it might be missing some steps or some things may have to be done. Consider checking the other document in the repository for building the Windows version of the game.

In addition to the below steps, you will also need to download and build `jsoncpp-src-0.6.0-rc2`.

# Steps

 * **Compiling DROD:TCB from scratch on Windows XP**
 * 1 September 2008
 * by *silver Harloe*

This is a diary of what silver did to make compile DROD:TCB from scratch on Windows XP.
No guarantees, warantees, or promises of help come with.

(see http://forum.caravelgames.com/viewtopic.php?TopicID=15611&page=0#258890)

--------------------------------

1.  http://msdn.microsoft.com/vstudio/express/downloads/default.aspx
    Under "step 2," I selected "Go!" under "visual c++".
    This downloaded vcsetup.exe - I just put it on my desktop, I assume whereever is fine
2.  I ran vcsetup.exe. I personally chose not to send my setup experience to MS,
    and did not check options for MSDN express or the SQL Server thing. I assume that
    checking these or not is unimportant for DROD.
3.  After install it brought up a box with two links: one to get updates, one to register.
    I selected register, which brought up a webpage. Since I do not already have a
    "Windows Live" ID, I signed up for one. It was nice enough to remember I was trying
    to register VC++ after signing up, and so I registered on the next page.
    It sent me three emails, 2 of which yahoo marked as "bulk". I confirmed my email
    address with the link from the second email, and finished my registration with
    the link from the third email. I was sent a 14 character registration code.
    The Windows Update link is just the generic OS upgrade and not VS specific, so
    I ignored it and closed the (finished) installer.
4.  Back on http://msdn.microsoft.com/vstudio/express/downloads/default.aspx
    Under "step 3", I selected "Download Visual C++ 2005 Express SP1" (since I'm not using Vista,
    I ignored the Vista related section below). This downloaded VS80sp1-KB926748-X86-INTL.exe,
    again I just put it on my desktop and ran it. ("ok," "i agree," (wait), "ok.")
5.  From the Start Menu, I navigated to and ran Visual C++ 2005 Express Edition. Once it was
    done with first time stuff, I went to "Help" and selected "Register Product" and entered
    the code from step 3.
    My code was invalid (possibly because I took too long to get here after step 3), so I
    selected "register now" and this time did have a Windows Live ID, so I logged in and it
    gave me a new key, which worked.
    I selected "no" to the anonymous survey, but I assume this is moot.

**Okay, now I have VC++ Express installed and registered, but I will be wanting to make
standalone executables, so I need to do some more installation...**

6.  _using Internet Explorer_, I went to
    http://msdn.microsoft.com/vstudio/express/visualc/usingpsdk/
    Under "step 2," I selected "Platform SDK"
    On the next page, since I have genuine Windows, I hit the "Continue" button,
    and on the next page I hit the "Download" button next to **PSDK-x86.exe**.
    I went ahead and downloaded to my desktop.
7.  I ran **PSDK-x86.exe**... wait for installer to work,
        next, I agree, next, typical, next, next, next...
    then I read a few dozen pages of the top novel on my stack of books...
    after a long long time, this installed it to the default path of
    `C:\Program Files\Microsoft Platform SDK for Windows Server 2003 R2`.
8.  In my favorite text editor, I opened up the file
    `C:\Program Files\Microsoft Visual Studio 8\VC\vcpackages\VCProjectEngine.Dll.Express.Config`
    I edited the Inlude="" line by adding this inside the quotes, at the end:
        `;C:\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Include;C:\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Include\mfc`
    I edited the Library="" line similarly, adding:
        `;C:\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Lib`
    And the Path="" line:
        `;C:\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Bin`
    Saved the file and exited.
9.  I opened up a command prompt window and typed:
        `cd "%USERPROFILE%\Local Settings\Application Data\Microsoft\VCExpress\8.0"`
    then did a "dir". Had the file "vccomponents.dat" existed, I would have removed it.
10. Back to my favorite editor, I opened up the file
    `C:\Program Files\Microsoft Visual Studio 8\VC\VCProjectDefaults;corewin_express.vsprops`
    I edited the AdditionalDependencies line from:
        `AdditionalDependencies="kernel32.lib"`
    to:
        `AdditionalDependencies="kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib"`
    Saved the file and then...
11. Opened up the file
    `C:\Program Files\Microsoft Visual Studio 8\VC\VCWizards\AppWiz\Generic\Application\html\1033\AppSettings.htm`
    I searched for `WIN_APP.disabled` and changed it from `true` to `false`. Right below it, I also set to `false`.
    `WIN_APP_LABEL.disabled`, `DLL_APP.disabled`, `DLL_APP_LABEL.disabled`.
    Saved the file and exited.

In theory I can now compile standalone executable windows apps. On to the DROD Specific stuff.

12. http://forum.caravelgames.com/viewsitepage.php?id=98165
    Downloaded the 3.x source to my desktop (today it was 3.2.1.84),
    and unzipped it a folder I made there, so I have a `Desktop\drod\DROD32Source` folder
13. I opened up `Desktop\drod\DROD32Source\Master\Master.sln` in my favorite text editor (not yet in VC++)
    and edited line 22, wherein it says `..\..\BackEndLib\BackEndLib.vcproj` to just `..\BackEndLib\BackEndLib.vcproj`
    and edited line 24, making the same edit (remove ..\) for FrontEndLib
    then removed lines 16-21 (the CaravelNet Project, since source for that was not included)
    Saved the file and exited.

Let's start getting third party software...

14. I made the directory Desktop\drod\ThirdParty
15. downloaded http://curl.haxx.se/download/libcurl-7.15.2-win32-msvc.zip to that folder
    and extraced to `Desktop\drod\ThirdParty\libcurl-7.15.2-win32-msvc`
16. http://sourceforge.net/project/showfiles.php?group_id=10127&package_id=11277
    selected "expat_win32bin_2_0_1.exe"
    downloaded to `Desktop\drod\ThirdParty`
    ran it, but changed the extract path to `Desktop\drod\ThirdParty\Expat-2.0.0`
17. downloaded http://www.fmod.org/index.php/release/version/fmodapi375win.zip
    and extracted to `Desktop\drod\ThirdParty\fmodapi375win`
18. http://sourceforge.net/project/showfiles.php?group_id=5624&package_id=5683
    had to scroll down and click on 1.2.25 to find the version used in drod, but found a link to
    `lpng1225.zip`
    extracted it to `Desktop\drod\ThirdParty\lpng1225`
19. downloaded ftp://ftp.simtel.net/pub/simtelnet/msdos/graphics/jpegsr6.zip
    and extracted to `Desktop\drod\ThirdParty\jpeg-6b`
20. downloaded http://downloads.xiph.org/releases/theora/libtheora-1.0beta2.zip
    and extracted to `Desktop\drod\ThirdParty\libtheora-1.0beta2`
21. downloaded http://www.equi4.com/pub/mk/metakit-2.4.9.7.tar.gz
    and extracted to `Desktop\drod\ThirdParty\metakit-2.4.9.7`
    (I had previously downloaded/installed gzip and tar binaries for windows)
22. downloaded http://www.libsdl.org/release/SDL-devel-1.2.12-VC6.zip
    and extracted to `Desktop\drod\SDL-1.2.12`
23. downloaded http://www.libsdl.org/projects/SDL_ttf/release/SDL_ttf-devel-2.0.8-VC6.zip
    and extracted to `Desktop\drod\ThirdParty\SDL_ttf-2.0.8`
24. http://sourceforge.net/project/showfiles.php?group_id=3157&package_id=3121
    had to scroll down and click on 1.2.25 to find the version used in drod, but found a link to
    `ft214.zip`
    extracted it to `Desktop\drod\ThirdParty\freetype-2.1.4`
25. downloaded http://www.zlib.net/zlib123.zip
    extracted it to Desktop\drop\zlib (slight pain here: they didn't include the top-level
    directory in the zip file, so I had to make the directory and extract all the zip contents
    to that to prevent polluting ThirdParty)
26. http://downloads.xiph.org/releases/ogg/libogg-1.1.3.zip
    and extracted to `Desktop\drod\ThirdParty\libogg-1.1.3`
27. http://downloads.xiph.org/releases/vorbis/libvorbis-1.2.0.zip
    and extracted to `Desktop\drod\ThirdParty\libvorbis-1.2.0`

Okay, now we have everything, but not all in useful form.
I think the goal here is to get all these built up so that I have one or more .lib and .h files.
(at least one per third party lib)

these are built for me: `cURL`, `Expat`, `fmod`, `SDL`, `SDL_ttf`. so on to build the rest.

28. double-clicked Desktop\drod\ThirdParty\metakit-2.4.9.7\win\msvc70\mksrc.sln
    that brought up the "upgrade wizard" which I let run (with backups).
    In the top menu I selected "Build" and drug down to "Configuration Manager..."
        In the resulting box, in the upper right was a dropdown labelled "Active solution configuration:"
        I dropped it down to "Debug", then hit the "Close" button
    in the solution explorer on the left, I right-clicked the "mklib" project and selected "Build".
    Then in the top menu I selected "Build" and drug down to "Configuration Manager..."
        In the resulting box, in the upper right was a dropdown labelled "Active solution configuration:"
        I dropped it down to "Release", then hit the "Close" button
    in the solution explorer on the left, I right-clicked the "mklib" project and selected "Build" again
    it built with some warnings, it seems I'm now done with metakit.
29. double-clicked `Desktop\drod\ThirdParty\lpng1225\projects\visualc71\libpng.sln`
    that brought up the "upgrade wizard" which I let run (with backups).
    In the top menu I selected "Build" and drug down to "Configuration Manager..."
        In the resulting box, in the upper right was a dropdown labelled "Active solution configuration:"
        I dropped it down to "LIB Release", then hit the "Close" button
    in the solution explorer on the left, I right-clicked "Solution 'libpng'" and selected "Build Solution".
    Then in the top menu I selected "Build" and drug down to "Configuration Manager..."
        In the resulting box, in the upper right was a dropdown labelled "Active solution configuration:"
        I dropped it down to "LIB Debug", then hit the "Close" button
    in the solution explorer on the left, I right-clicked "Solution 'libpng'" and selected "Build Solution" again.
    zlib and libpng both built, but pngtest failed. I ignored that (the problem is missing pngtest.png)
    (side note: copying `pngtest.png` from `Desktop\drod\ThirdParty\lpng1225` to
    `Desktop\drod\ThirdParty\lpng1225\projects\visualc71\Win32_LIB_Release\Test` and manually running `pngtest.exe`
    resulted in success)
    it seems I'm now done with libpng and got zlib as a bonus.
30. double-clicked `Desktop\drod\ThirdParty\freetype-2.1.4\builds\win32\visualc\freetype.dsw`
    that brought up a converstion wizard, so I hit "yes to all".
    in the solution explorer on the left, I right-clicked "Solution 'freetype'" and selected "Build Solution".
    it built with some warnings, it seems I'm now done with freetype.
31. double-clicked `Desktop\drod\ThirdParty\libogg-1.1.3\win32\ogg.dsw`
    that brought up a converstion wizard, so I hit "yes to all".
    In the solution explorer on the left, I right-clicked "ogg_static" and selected "Build".
    Then in the top menu I selected "Build" and drug down to "Configuration Manager..."
        In the resulting box, in the upper right was a dropdown labelled "Active solution configuration:"
        I dropped it down to "Release", then hit the "Close" button
    in the solution explorer on the left, I right-clicked "ogg_static" and selected "Build" again
    it built with no warnings, so it seems I'm now done with libogg (though I possibly
    don't need this for DROD directly, I do need it for the next four steps)
32. double-clicked `Desktop\drod\ThirdParty\libvorbis-1.2.0\win32\vorbisenc_static.dsp`
    that brought up a converstion wizard, so I hit "yes to all".
    in the solution explorer on the left, I right-clicked on the "vorbisenc_static" project and selected "Properties"
    on the left side of that window, I selected "C/C++"
        in the top line on the right-hand box ("Additional Include Directories"),
        I changed "..\..\ogg\include" to "..\..\libogg-1.1.3\include"
    then I clicked "ok" to dismiss the properties window,
    In the solution explorer on the left, I right-clicked "vorbisenc_static" and selected "Build".
       this asked me to save "vorbisenc_static.sln", so I hit "Save" on that window.
    Then in the top menu I selected "Build" and drug down to "Configuration Manager..."
        In the resulting box, in the upper right was a dropdown labelled "Active solution configuration:"
        I dropped it down to "Release", then hit the "Close" button
    in the solution explorer on the left, I right-clicked "vorbisenc_static" and selected "Build" again
    it built with some warnings, so it seems I'm now done with vorbisenc.
33. double-clicked `Desktop\drod\ThirdParty\libvorbis-1.2.0\win32\vorbisfile_static.dsp`
    that brought up a converstion wizard, so I hit "yes to all".
    in the solution explorer on the left, I right-clicked on the "vorbisfile_static" project and selected "Properties"
    on the left side of that window, I selected "C/C++"
        in the top line on the right-hand box ("Additional Include Directories"),
        I changed "..\..\ogg\include" to "..\..\libogg-1.1.3\include"
    then I clicked "ok" to dismiss the properties window,
    In the solution explorer on the left, I right-clicked "vorbisfile_static" and selected "Build".
       this asked me to save "vorbisfile_static.sln", so I hit "Save" on that window.
    Then in the top menu I selected "Build" and drug down to "Configuration Manager..."
        In the resulting box, in the upper right was a dropdown labelled "Active solution configuration:"
        I dropped it down to "Release", then hit the "Close" button
    in the solution explorer on the left, I right-clicked "vorbisfile_static" and selected "Build" again
    it built with some warnings, so it seems I'm now done with vorbisfile.
34. double-clicked `Desktop\drod\ThirdParty\libvorbis-1.2.0\win32\vorbis_static.dsp`
    that brought up a converstion wizard, so I hit "yes to all".
    in the solution explorer on the left, I right-clicked on the "vorbis_static" project and selected "Properties"
    on the left side of that window, I selected "C/C++"
        in the top line on the right-hand box ("Additional Include Directories"),
        I changed "..\..\ogg\include" to "..\..\libogg-1.1.3\include"
    then I clicked "ok" to dismiss the properties window,
    In the solution explorer on the left, I right-clicked "vorbisfile_static" and selected "Build".
       this asked me to save "vorbis_static.sln", so I hit "Save" on that window.
    Then in the top menu I selected "Build" and drug down to "Configuration Manager..."
        In the resulting box, in the upper right was a dropdown labelled "Active solution configuration:"
        I dropped it down to "Release", then hit the "Close" button
    in the solution explorer on the left, I right-clicked "vorbisfile_static" and selected "Build" again
    it built with some warnings, so it seems I'm now done with vorbis.
35. double-clicked `Desktop\drod\ThirdParty\libtheora-1.0beta2\win32\theora_static.dsp`
    that brought up a converstion wizard, so I hit "yes to all".
    in the solution explorer on the left, I right-clicked on the "theora_static" project and selected "Properties"
    on the left side of that window, I selected "C/C++"
        in the top line on the right-hand box ("Additional Include Directories"),
        I changed "..\..\ogg\include,..\..\theora\include" to "..\..\libogg-1.1.3\include,..\..\libtheora-1.0beta2\include,..\..\libtheora-1.0beta2\lib\enc"
    then on the left side of that window, I clicked on "Preprocessor" under "C/C++"
        in the top line on the right-hand box ("Preprocessor Definitions"),
        I added ";TH_REALLY_NO_ASSEMBLY" to the end of the existing entry
    then on the left side of that window, I clicked on "Librarian"
        I clicked in the top line on the right-hand box ("Additional Dependencies")
        and entered the path "..\..\libogg-1.1.3\win32\Static_Debug\ogg_static_d.lib"
    then I clicked "ok" to dismiss the properties window,
    In the solution explorer on the left, I right-clicked on the "theora_static" project and selected "Build"
        this asked me to save "theora_static.sln", so I hit "Save" on that window.
    in the solution explorer on the left, I right-clicked on the "theora_static" project and selected "Properties"
    in the upper left-hand side, I selected "Configuration" and drug down to "Release"
    on the left side of that window, I selected "C/C++"
        in the top line on the right-hand box ("Additional Include Directories"),
        I changed "..\..\ogg\include,..\..\theora\include" to "..\..\libogg-1.1.3\include,..\..\libtheora-1.0beta2\include,..\..\libtheora-1.0beta2\lib\enc"
    then on the left side of that window, I clicked on "Preprocessor" under "C/C++"
        in the top line on the right-hand box ("Preprocessor Definitions"),
        I added ";TH_REALLY_NO_ASSEMBLY" to the end of the existing entry
    then on the left side of that window, I clicked on "Librarian"
        I clicked in the top line on the right-hand box ("Additional Dependencies")
        and entered the path "..\..\libogg-1.1.3\win32\Static_Debug\ogg_static_d.lib"
    then I clicked "ok" to dismiss the properties window,
    Then in the top menu I selected "Build" and drug down to "Configuration Manager..."
        In the resulting box, in the upper right was a dropdown labelled "Active solution configuration:"
        I dropped it down to "Release", then hit the "Close" button
    In the solution explorer on the left, I right-clicked on the "theora_static" project and selected "Build" again
    it built with some warnings, it seems I'm now done with libtheora.
36. I opened up a command prompt and typed the following commands:
        `cd Desktop\drod\ThirdParty\jpeg-6b`
        `C:\Program Files\Microsoft Visual Studio 8\Common7\Tools\vsvars32.bat`
        `set INCLUDE=%INCLUDE%;C:\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Include`
        `set LIB=%LIB%;C:\Program Files\Microsoft Platform SDK for Windows Server 2003 R2\Lib`
        `copy jconfig.vc jconfig.h`
        `nmake /f makefile.vc nodebug=1`
    it seems I'm now done with libjpeg.

now all my third-party libraries are built, it's time to move on to actually compiling DROD itself. yay.

37. I started up Visual C++ and on the top menu selected "Tools" and then scrolled down to "Options".
    In the resulting dialog box, in the navigator on the left, I opened up "Projects and Solutions" and under
    that selected "VC++ Directories". Once I selected that, there was a drop-down on the upper-right of the dialog
    box. I scrolled down to "Include files" in that. In directory list I started clicking the blank line below the
    already existing directories, then pasting in each of these lines, one at a time:
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\Expat-2.0.0\Source\lib`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\SDL-1.2.12\include`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\SDL_ttf-2.0.8\include`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\fmodapi375win\api\inc`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\jpeg-6b`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\libcurl-7.15.2-win32-msvc\include`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\libogg-1.1.3\include`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\libvorbis-1.2.0\include`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\libtheora-1.0beta2\include`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\lpng1225`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\metakit-2.4.9.7\include`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\zlib`
38. With the dialog still open, I went back to the drop-down in the upper-right and scrolled it down to
    "Library files". Using the same click and paste one line at a time (darn them for refusing multi-line pastes),
    I added all these lines to that list:
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\Expat-2.0.0\Bin`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\SDL-1.2.12\lib`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\SDL_ttf-2.0.8\lib`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\fmodapi375win\api\lib`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\freetype-2.1.4\objs`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\jpeg-6b`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\libcurl-7.15.2-win32-msvc`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\libogg-1.1.3\win32\Static_Release`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\libogg-1.1.3\win32\Static_Debug`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\libvorbis-1.2.0\win32\VorbisEnc_Static_Release`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\libvorbis-1.2.0\win32\VorbisEnc_Static_Debug`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\libvorbis-1.2.0\win32\VorbisFile_Static_Release`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\libvorbis-1.2.0\win32\VorbisFile_Static_Debug`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\libvorbis-1.2.0\win32\Vorbis_Static_Release`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\libvorbis-1.2.0\win32\Vorbis_Static_Debug`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\libtheora-1.0beta2\win32\Static_Release`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\libtheora-1.0beta2\win32\Static_Debug`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\lpng1225\projects\visualc71\Win32_LIB_Release`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\lpng1225\projects\visualc71\Win32_LIB_Release\ZLib`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\lpng1225\projects\visualc71\Win32_LIB_Debug`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\lpng1225\projects\visualc71\Win32_LIB_Debug\ZLib`
        `C:\Documents and Settings\silver Harloe\Desktop\drod\ThirdParty\metakit-2.4.9.7\builds`
    Then I went ahead and closed Visual C++ because I'm more comfortable with explorer than VC for doing the first part
    of the next step.
39. So, finally I explored to `Desktop\drod\DROD32Source\Master` and double-clicked on `Master.sln`. This opened up
    Visual C++ with the DROD solution ready to go.
    I right-clicked on on the "drod" project in the navigator on the left, and scrolled down to "Properties"
        In the resulting dialog box, in the navigator on the left, I opened up "Configuration Properties",
        and under that "C/C++", and under that selected "Preprocessor". In the box on the right, I edited the
        "Preprocessor Definitions" and removed the ";CARAVELBUILD" from the end.
    In the upper left-hand side, I selected "Configuration" and drug down to "Release"
        Then once again made sure "C/C++", and under that "Preprocessor" was selected. In the box on the right, I edited the
        "Preprocessor Definitions" and removed the ";CARAVELBUILD" from the end.
    I right-clicked on on the "DRODUtil" project in the navigator on the left, and scrolled down to "Properties"
        In the resulting dialog box, in the navigator on the left, I opened up "Configuration Properties",
        and under that "Linker", and under that selected "Input". In the box on the right, I edited the
        "Additional Dependencies" and edited the part where it says "mk4vc60s_d.lib" to read "mk4vc70s_d.lib",
        and edited "libjpegd.lib" to read "libjpeg.lib"
    I right-clicked on on the "drod" project in the navigator on the left, and scrolled down to "Properties"
        In the resulting dialog box, in the navigator on the left, I opened up "Configuration Properties",
        and under that "Linker", and under that selected "Input". In the box on the right, I edited the
        "Additional Dependencies" and edited the part where it says "libjpegd.lib" to read "libjpeg.lib"
    I right-clicked "Solution 'Master'" in the navigator on the left, and selected "Build Solution".
    Pause for dramatic effect (and because it took a while to compile).

    I got an error compiling DRODUtil, but that's an optional, separate .exe - drod.exe was built and works -
    Also, when I rebuild the Master solution as a "release," DRODUtil built just fine.

Time to test.

40. I went http://www.caravelgames.com/distfiles/DRODTCBDemoSetup.exe and saved that file to the Desktop\drod folder
    Then I clicked the installer but changed the install path to "C:\Documents and Settings\silver Harloe\Desktop\drod\Test Build Debug",
    and changed the Start Menu folder to "DROD - TCB Test Build", and unclicked "create a desktop icon". When it was done installing,
    I did not allow it to launch yet.
41. In the folder `C:\Documents and Settings\silver Harloe\Desktop\drod\Test Build Debug` I created a subfolder called "orig" and
    moved drod.exe into it, then I copied in the drod.exe from "C:\Documents and Settings\silver Harloe\Desktop\drod\DROD32Source\DROD\Debug"
42. I ran and played the DROD demo.
