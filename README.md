# Build Instructions for macos arm

If you somehow don't have xcode commandline tools yet, install them with

```
xcode-select --install
```

Install binary from
[Connector-cpp](https://dev.mysql.com/downloads/connector/cpp/)

Find out where the folder gets installed. I will call this `CONN_LOC` from now on. Mine was installed at `/usr/local/mysql-connector-c++-9.5.0`

Run these commands to correctly link the library with it's openssl libraries (for some reason they didn't set it up to find the files correctly even though they're right next to each other):

```
cd CONN_LOC/lib64

sudo install_name_tool -change libssl.3.dylib \
  @rpath/libssl.3.dylib \
  CONN_LOC/lib64/libmysqlcppconn.10.dylib

sudo install_name_tool -change libcrypto.3.dylib \
  @rpath/libcrypto.3.dylib \
  CON_LOC/lib64/libmysqlcppconn.10.dylib
```

Codesign the libraries so macos doesn't annilate your program since it detects modifications

```
sudo xattr -dr com.apple.quarantine .
sudo codesign --force --deep --sign - libmysqlcppconn.10.dylib
sudo codesign --force --deep --sign - libssl.3.dylib
sudo codesign --force --deep --sign - libcrypto.3.dylib
```

Now you're finally ready to build the program. Navigate back to your directory. Make sure the MYSQL variable is set to CON_LOC in `build.sh` and then build your program with

```
./build.sh
```

## Configure vscode

Replace the CONN_LOC with your actual CONN_LOC in `.vscode/c_cpp_properties.json` so that vscode's intellisense can find the library.
