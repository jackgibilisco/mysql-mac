MYSQL=CONN_LOC

clang++ sql.cpp -o app \
    -std=c++17 -stdlib=libc++ \
    -I ${MYSQL}/include \
    -I ${MYSQL}/include/jdbc \
    -L ${MYSQL}/lib64 \
    -lmysqlcppconn \
    -Wl,-rpath,${MYSQL}/lib64