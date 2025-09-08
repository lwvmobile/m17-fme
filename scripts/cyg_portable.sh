#! /bin/bash
#
cdir=$(pwd)
clear
printf "M17-FME Project M17 - Florida Man Edition Portable Cygwin Creator\n"
printf "Only use this in a Cygwin64 Environment. Will not work in a Linux Environment!\n"
read -p "Do you want to make a portable copy? y/N " RELEASE
RELEASE=$(printf "$RELEASE"|tr '[:upper:]' '[:lower:]')
if [ "$RELEASE" = "y" ]; then

  printf "Copying files to m17-fme-portable folder and making zip file. Please wait!\n"
  cd ~

  #create a m17-fme-portable folder and copy all the needed items for a portable release version
  mkdir m17-fme-portable
  cd m17-fme-portable
  #all of these folders may not be necessary
  mkdir sourcecode
  mkdir bin
  mkdir dev
  cd dev
  mkdir shm
  cd ..
  mkdir m17-fme
  mkdir logs
  mkdir m17wav
  mkdir etc
  mkdir home
  mkdir lib
  mkdir tmp
  mkdir usr
  cd usr
  mkdir share
  cd share
  mkdir terminfo
  cd ~

  #start copying things

  #terminfo is needed for ncurses
  cp -r /usr/share/terminfo/* m17-fme-portable/usr/share/terminfo

  #copy required items from the root bin folder to release bin folder
  cp /bin/ls.exe m17-fme-portable/bin/
  cp /bin/bash.exe m17-fme-portable/bin/
  cp /bin/ncurses6-config m17-fme-portable/bin/
  cp /bin/ncursesw6-config m17-fme-portable/bin/
  cp /bin/pacat.exe m17-fme-portable/bin/
  cp /bin/pactl.exe m17-fme-portable/bin/
  cp /bin/padsp m17-fme-portable/bin/
  cp /bin/pamon m17-fme-portable/bin/
  cp /bin/paplay m17-fme-portable/bin/
  cp /bin/parec m17-fme-portable/bin/
  cp /bin/parecord m17-fme-portable/bin/
  cp /bin/pulseaudio.exe m17-fme-portable/bin/
  cp /bin/tty.exe m17-fme-portable/bin/

  #start copying things from the etc folder
  cp -r /etc/pulse/ m17-fme-portable/etc/

  #start copying things to the lib folder
  cp -r /lib/pulseaudio/ m17-fme-portable/lib/

  #start copying things into the m17-fme folder
  cp m17-fme/build/m17-fme.exe m17-fme-portable/m17-fme/
  cp -r m17-fme/docs m17-fme-portable/m17-fme/
  cp -r m17-fme/scripts m17-fme-portable/m17-fme
  cp -r m17-fme/samples m17-fme-portable/m17-fme
  cp -r m17-fme/key m17-fme-portable/m17-fme
  cp .bash_profile m17-fme-portable/m17-fme/
  cp .bashrc m17-fme-portable/m17-fme/
  cp .inputrc m17-fme-portable/m17-fme/
  cp .profile m17-fme-portable/m17-fme/

  #move (cut) the bat files to the portable folder root and delete the other ones so users won't get confused
  mv m17-fme-portable/m17-fme/scripts/cygwin_bat/start-m17-fme.bat m17-fme-portable/
  mv m17-fme-portable/m17-fme/scripts/cygwin_bat/example_options.txt m17-fme-portable/
  mv m17-fme-portable/m17-fme/scripts/cygwin_bat/complete_usage_options.txt m17-fme-portable/

  #change .bat files permissions to be read/write/executable by all users (for some reason, this is not preserved)
  chmod 777 m17-fme-portable/*.bat

  #copy the license, copyright, source code, and things that should be included in all portable releases
  cp m17-fme/LICENSE m17-fme-portable/sourcecode/
  cp m17-fme/README.md m17-fme-portable/sourcecode/

  #fix this after testing things
  cp m17-fme/CMakeLists.txt m17-fme-portable/sourcecode/
  cp m17-fme/.gitignore m17-fme-portable/sourcecode/
  cp m17-fme/.gitmodules m17-fme-portable/sourcecode/
  cp -r m17-fme/src m17-fme-portable/sourcecode/
  cp -r m17-fme/include m17-fme-portable/sourcecode/
  cp -r m17-fme/cmake m17-fme-portable/sourcecode/

  #compiled items into release m17-fme folder
  cp codec2/build/src/cygcodec2* m17-fme-portable/m17-fme/
#   cp rtl-sdr/build/src/cyg* m17-fme-portable/m17-fme/ #re-enable if RTL support is ever added (good chance of it in the future)

  #bin items into release m17-fme folder (uisng * where version numbering may exist, those may be upgraded if hard coded)
  cp /bin/bash.exe m17-fme-portable/m17-fme/
  cp /bin/cut.exe m17-fme-portable/m17-fme/
  cp /bin/cygao* m17-fme-portable/m17-fme/
  cp /bin/cygarchive* m17-fme-portable/m17-fme/
  cp /bin/cygargp* m17-fme-portable/m17-fme/
  cp /bin/cygassuan* m17-fme-portable/m17-fme/
  cp /bin/cygasyncns* m17-fme-portable/m17-fme/
  cp /bin/cygatomic* m17-fme-portable/m17-fme/
  cp /bin/cygattr* m17-fme-portable/m17-fme/
  cp /bin/cygavahi-client* m17-fme-portable/m17-fme/
  cp /bin/cygavahi-common* m17-fme-portable/m17-fme/
  cp /bin/cygavahi-wrap* m17-fme-portable/m17-fme/
  cp /bin/cygblkid* m17-fme-portable/m17-fme/
  cp /bin/cygbrotlicommon* m17-fme-portable/m17-fme/
  cp /bin/cygbrotlidec* m17-fme-portable/m17-fme/
  cp /bin/cygbz2* m17-fme-portable/m17-fme/
  cp /bin/cygcares* m17-fme-portable/m17-fme/
  cp /bin/cygcheck.exe m17-fme-portable/m17-fme/
  cp /bin/cygcli.dll m17-fme-portable/m17-fme/
  cp /bin/cygcom_err* m17-fme-portable/m17-fme/
  cp /bin/cygcrypt* m17-fme-portable/m17-fme/
  cp /bin/cygcrypto* m17-fme-portable/m17-fme/
  cp /bin/cygcurl* m17-fme-portable/m17-fme/
  cp /bin/cygdb_cxx* m17-fme-portable/m17-fme/
  cp /bin/cygdb_sql* m17-fme-portable/m17-fme/
  cp /bin/cygdb* m17-fme-portable/m17-fme/
  cp /bin/cygdbus* m17-fme-portable/m17-fme/
  cp /bin/cygedit* m17-fme-portable/m17-fme/
  cp /bin/cygexpat* m17-fme-portable/m17-fme/
  # cp /bin/cygfam* m17-fme-portable/m17-fme/ #cannot stat, must have been removed?
  cp /bin/cygfdisk* m17-fme-portable/m17-fme/
  cp /bin/cygffi* m17-fme-portable/m17-fme/
  cp /bin/cygfftw3_threads* m17-fme-portable/m17-fme/
  cp /bin/cygfftw3* m17-fme-portable/m17-fme/
  cp /bin/cygfftw3f_threads* m17-fme-portable/m17-fme/
  cp /bin/cygfftw3f* m17-fme-portable/m17-fme/
  cp /bin/cygfftw3l_threads* m17-fme-portable/m17-fme/
  cp /bin/cygfftw3l* m17-fme-portable/m17-fme/
  cp /bin/cygfido2* m17-fme-portable/m17-fme/
  cp /bin/cygFLAC* m17-fme-portable/m17-fme/
  cp /bin/cygformw* m17-fme-portable/m17-fme/
  cp /bin/cyggc* m17-fme-portable/m17-fme/
  cp /bin/cyggdbm_compat* m17-fme-portable/m17-fme/
  cp /bin/cyggdbm*.dll m17-fme-portable/m17-fme/
  cp /bin/cyggio* m17-fme-portable/m17-fme/
  cp /bin/cygglib* m17-fme-portable/m17-fme/
  cp /bin/cyggmodule* m17-fme-portable/m17-fme/
  cp /bin/cyggmp* m17-fme-portable/m17-fme/
  cp /bin/cyggnutls* m17-fme-portable/m17-fme/
  cp /bin/cyggobject* m17-fme-portable/m17-fme/
  cp /bin/cyggomp* m17-fme-portable/m17-fme/
  cp /bin/cyggpg-error* m17-fme-portable/m17-fme/
  cp /bin/cyggpgme* m17-fme-portable/m17-fme/
  cp /bin/cyggsasl* m17-fme-portable/m17-fme/
  cp /bin/cyggsm* m17-fme-portable/m17-fme/
  cp /bin/cyggssapi_krb5* m17-fme-portable/m17-fme/
  cp /bin/cyggthread* m17-fme-portable/m17-fme/
  cp /bin/cygguile* m17-fme-portable/m17-fme/
  cp /bin/cyghistory* m17-fme-portable/m17-fme/
  cp /bin/cyghogweed* m17-fme-portable/m17-fme/
  cp /bin/cygICE* m17-fme-portable/m17-fme/
  cp /bin/cygiconv* m17-fme-portable/m17-fme/
  cp /bin/cygid3tag* m17-fme-portable/m17-fme/
  cp /bin/cygidn* m17-fme-portable/m17-fme/
  cp /bin/cygintl* m17-fme-portable/m17-fme/
  cp /bin/cygisl* m17-fme-portable/m17-fme/
  cp /bin/cygjsoncpp* m17-fme-portable/m17-fme/
  cp /bin/cygk5crypto* m17-fme-portable/m17-fme/
  cp /bin/cygkrb5* m17-fme-portable/m17-fme/
  cp /bin/cyglber* m17-fme-portable/m17-fme/
  cp /bin/cygldap* m17-fme-portable/m17-fme/
  cp /bin/cygltdl* m17-fme-portable/m17-fme/
  cp /bin/cyglz* m17-fme-portable/m17-fme/
  cp /bin/cygma* m17-fme-portable/m17-fme/
  cp /bin/cygmenuw* m17-fme-portable/m17-fme/
  cp /bin/cygmeta* m17-fme-portable/m17-fme/
  cp /bin/cygmp* m17-fme-portable/m17-fme/
  cp /bin/cygncurses* m17-fme-portable/m17-fme/
  cp /bin/cygnettle* m17-fme-portable/m17-fme/
  cp /bin/cygnghttp* m17-fme-portable/m17-fme/
  cp /bin/cygntlm* m17-fme-portable/m17-fme/
  cp /bin/cygobjc* m17-fme-portable/m17-fme/
  cp /bin/cygogg* m17-fme-portable/m17-fme/
  cp /bin/cygopus* m17-fme-portable/m17-fme/
  cp /bin/cygorc* m17-fme-portable/m17-fme/
  cp /bin/cygp11* m17-fme-portable/m17-fme/
  cp /bin/cygpanel* m17-fme-portable/m17-fme/
  cp /bin/cygpath.exe m17-fme-portable/m17-fme/
  cp /bin/cygpcre* m17-fme-portable/m17-fme/
  cp /bin/cygperl* m17-fme-portable/m17-fme/
  cp /bin/cygpipe* m17-fme-portable/m17-fme/
  cp /bin/cygpkg* m17-fme-portable/m17-fme/
  cp /bin/cygpng* m17-fme-portable/m17-fme/
  cp /bin/cygpopt* m17-fme-portable/m17-fme/
  cp /bin/cygprotocol* m17-fme-portable/m17-fme/
  cp /bin/cygpsl* m17-fme-portable/m17-fme/
  cp /bin/cygpulse* m17-fme-portable/m17-fme/
  cp /bin/cygquadmath* m17-fme-portable/m17-fme/
  cp /bin/cygreadline* m17-fme-portable/m17-fme/
  cp /bin/cygrhash* m17-fme-portable/m17-fme/
  cp /bin/cygrunsrv.exe m17-fme-portable/m17-fme/
  cp /bin/cygsamplerate* m17-fme-portable/m17-fme/
  cp /bin/cygsasl* m17-fme-portable/m17-fme/
  cp /bin/cygserver* m17-fme-portable/m17-fme/
  cp /bin/cygSM* m17-fme-portable/m17-fme/
  cp /bin/cygsmart* m17-fme-portable/m17-fme/
  cp /bin/cygsndfile* m17-fme-portable/m17-fme/
  cp /bin/cygsox* m17-fme-portable/m17-fme/
  cp /bin/cygspeex* m17-fme-portable/m17-fme/
  cp /bin/cygsqlite* m17-fme-portable/m17-fme/
  cp /bin/cygssh* m17-fme-portable/m17-fme/
  cp /bin/cygssl* m17-fme-portable/m17-fme/
  cp /bin/cygstart.exe m17-fme-portable/m17-fme/
  cp /bin/cygstdc* m17-fme-portable/m17-fme/
  cp /bin/cygtasn* m17-fme-portable/m17-fme/
  cp /bin/cygtdb* m17-fme-portable/m17-fme/
  cp /bin/cygticw* m17-fme-portable/m17-fme/
  cp /bin/cygtwo* m17-fme-portable/m17-fme/
  cp /bin/cygunistring* m17-fme-portable/m17-fme/
  cp /bin/cygusb* m17-fme-portable/m17-fme/
  cp /bin/cyguuid* m17-fme-portable/m17-fme/
  cp /bin/cyguv* m17-fme-portable/m17-fme/
  cp /bin/cygvorbis* m17-fme-portable/m17-fme/
  cp /bin/cygwavpack* m17-fme-portable/m17-fme/
  cp /bin/cygwin1.dll m17-fme-portable/m17-fme/
  cp /bin/cygwrap* m17-fme-portable/m17-fme/
  cp /bin/cygX* m17-fme-portable/m17-fme/
  cp /bin/cygx* m17-fme-portable/m17-fme/
  cp /bin/cygz* m17-fme-portable/m17-fme/
  cp /bin/grep.exe m17-fme-portable/m17-fme/
  cp /bin/ls.exe m17-fme-portable/m17-fme/
  cp /bin/mintty.exe m17-fme-portable/m17-fme/
  cp /bin/pacat.exe m17-fme-portable/m17-fme/
  cp /bin/pacmd.exe m17-fme-portable/m17-fme/
  cp /bin/pactl.exe m17-fme-portable/m17-fme/
  cp /bin/padsp m17-fme-portable/m17-fme/
  cp /bin/pamon m17-fme-portable/m17-fme/
  cp /bin/paplay m17-fme-portable/m17-fme/
  cp /bin/parec m17-fme-portable/m17-fme/
  cp /bin/pulseaudio.exe m17-fme-portable/m17-fme/
  cp /bin/tail.exe m17-fme-portable/m17-fme/
  cp /bin/touch.exe m17-fme-portable/m17-fme/
  cp /bin/tty.exe m17-fme-portable/m17-fme/

  zip -r m17-fme-x86-64-cygwin-portable.zip m17-fme-portable

fi
