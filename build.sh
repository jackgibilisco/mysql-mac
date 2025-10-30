MYSQL=/usr/local/mysql-connector-c++-9.5.0

clang++ sql.cpp -o app \
    -std=c++17 -stdlib=libc++ \
    -I ${MYSQL}/include \
    -I ${MYSQL}/include/jdbc \
    -L ${MYSQL}/lib64 \
    -lmysqlcppconn \
    -Wl,-rpath,${MYSQL}/lib64