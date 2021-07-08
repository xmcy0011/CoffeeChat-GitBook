#!/bin/bash

PROTOBUF=protobuf-3.17.3
CUR_DIR=

check_user() {
    if [ $(id -u) != "0" ]; then
        echo "Error: You must be root to run this script, please use root to install im"
        exit 1
    fi
}

get_cur_dir() {
    # Get the fully qualified path to the script
    case $0 in
        /*)
            SCRIPT="$0"
            ;;
        *)
            PWD_DIR=$(pwd);
            SCRIPT="${PWD_DIR}/$0"
            ;;
    esac
    # Resolve the true real path without any sym links.
    CHANGED=true
    while [ "X$CHANGED" != "X" ]
    do
        # Change spaces to ":" so the tokens can be parsed.
        SAFESCRIPT=`echo $SCRIPT | sed -e 's; ;:;g'`
        # Get the real path to this script, resolving any symbolic links
        TOKENS=`echo $SAFESCRIPT | sed -e 's;/; ;g'`
        REALPATH=
        for C in $TOKENS; do
            # Change any ":" in the token back to a space.
            C=`echo $C | sed -e 's;:; ;g'`
            REALPATH="$REALPATH/$C"
            # If REALPATH is a sym link, resolve it.  Loop for nested links.
            while [ -h "$REALPATH" ] ; do
                LS="`ls -ld "$REALPATH"`"
                LINK="`expr "$LS" : '.*-> \(.*\)$'`"
                if expr "$LINK" : '/.*' > /dev/null; then
                    # LINK is absolute.
                    REALPATH="$LINK"
                else
                    # LINK is relative.
                    REALPATH="`dirname "$REALPATH"`""/$LINK"
                fi
            done
        done

        if [ "$REALPATH" = "$SCRIPT" ]
        then
            CHANGED=""
        else
            SCRIPT="$REALPATH"
        fi
    done
    # Change the current directory to the location of the script
    CUR_DIR=$(dirname "${REALPATH}")
}

build_protobuf(){
    cd protobuf
    tar -xf $PROTOBUF.tar.gz
    cd $PROTOBUF
    ./configure --prefix=$CUR_DIR/protobuf
    make
    make install
    cd ..

    # 拷贝库和头文件
    #mkdir -p ../lib/
    #cp lib/libprotobuf-lite.a ../lib/
    #cp  -r include/* ../pb/

    # 拷贝到系统目录，这样其他项目使用的时候，只需要编译一次，比较方便
    mkdir /usr/local/protobuf
    cp -rf bin /usr/local/protobuf3
    cp -rf lib /usr/local/protobuf3
    cp -rf include /usr/local/protobuf3

    rm -rf protobuf-3.17.3
}

check_user
get_cur_dir
build_protobuf