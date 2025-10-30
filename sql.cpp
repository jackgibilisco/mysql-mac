// ================================================
//  MySQL C++ Connector (Legacy API) Example
//  --------------------------------------------
//  Demonstrates:
//   - Connecting to a MySQL server
//   - Creating schema & table
//   - Using prepared statements
//   - Using transactions (commit/rollback)
//   - Reading results from queries
// ================================================

// ====== System headers ======
#include <iostream>    // for std::cout, std::cerr
#include <memory>      // for std::unique_ptr (RAII)
#include <vector>      // for std::vector container
#include <string>      // for std::string
#include <iomanip>     // for std::setw, formatting output

// ====== MySQL Connector headers ======
// These come from the "include" directory of MySQL Connector/C++
#include <mysql_driver.h>                // for sql::mysql::get_mysql_driver_instance()
#include <cppconn/driver.h>              // defines sql::Driver class
#include <cppconn/exception.h>           // defines sql::SQLException for catching DB errors
#include <cppconn/connection.h>          // defines sql::Connection (represents a DB connection)
#include <cppconn/statement.h>           // defines sql::Statement (for SQL commands)
#include <cppconn/prepared_statement.h>  // defines sql::PreparedStatement (parameterized SQL)
#include <cppconn/resultset.h>           // defines sql::ResultSet (returned query results)

// ---------------------------------------------------------
// Struct: DbConfig
// Holds MySQL connection configuration info.
// Normally, you'd read these from environment variables
// or a config file, but here we hardcode them for simplicity.
// ---------------------------------------------------------
struct DbConfig {
    std::string host = "tcp://127.0.0.1:3306";  // TCP connection to localhost MySQL on port 3306
    std::string user = "root";                  // Username to log into MySQL
    std::string pass = "sinatra1";         // Password for that user
    std::string schema = "testdb";                // Database to use (will be created if missing)
};

// ---------------------------------------------------------
// Struct: User
// A simple C++ struct to represent a row in the "users" table
// ---------------------------------------------------------
struct User {
    int         id;    // user ID (primary key, auto-incremented)
    std::string name;  // user's name
    int         age;   // user's age
};

// ---------------------------------------------------------
// Helper function: printSqlError
// Prints all possible details of a sql::SQLException
// ---------------------------------------------------------
void printSqlError(const sql::SQLException& e, const std::string& where) {
    std::cerr << "[SQL ERROR @ " << where << "] "
        << e.what()                                  // human-readable error message
        << " | MySQL error code: " << e.getErrorCode() // numeric error code from MySQL
        << " | SQLState: " << e.getSQLStateCStr()      // 5-character SQLState diagnostic
        << "\n";
}

// ---------------------------------------------------------
// Function: ensureSchemaAndTables
// Ensures that the desired database and table exist.
// If not, creates them.
// ---------------------------------------------------------
void ensureSchemaAndTables(sql::Connection* con, const std::string& schema) {
    // Create a statement object (used for executing SQL without parameters)
    std::unique_ptr<sql::Statement> stmt(con->createStatement());

    // Create the database if it doesn't exist
    stmt->execute("CREATE DATABASE IF NOT EXISTS `" + schema + "`");

    // Switch to using that database
    con->setSchema(schema);

    // Create the users table (if not exists)
    stmt->execute(
        "CREATE TABLE IF NOT EXISTS users ("
        "  id INT AUTO_INCREMENT PRIMARY KEY,"   // unique row ID
        "  name VARCHAR(100) NOT NULL,"          // required string field
        "  age INT NULL,"                        // optional integer field
        "  UNIQUE KEY uq_users_name (name)"      // make name unique for demo purposes
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;"
    );
}

// ---------------------------------------------------------
// Function: insertUser
// Demonstrates an INSERT with a PreparedStatement.
// Returns the new auto-generated ID.
// ---------------------------------------------------------
int insertUser(sql::Connection* con, const User& u) {
    // Create a prepared statement with placeholders '?'
    std::unique_ptr<sql::PreparedStatement> ps(
        con->prepareStatement("INSERT INTO users(name, age) VALUES(?, ?)")
    );

    // Bind values to the placeholders (1-indexed)
    ps->setString(1, u.name);
    if (u.age == 0) ps->setNull(2, 0);  // handle NULL properly
    else ps->setInt(2, u.age);

    // Execute the SQL command (no resultset expected)
    ps->executeUpdate();

    // Retrieve the auto-generated ID of the inserted row
    std::unique_ptr<sql::Statement> s(con->createStatement());
    std::unique_ptr<sql::ResultSet> r(s->executeQuery("SELECT LAST_INSERT_ID()"));

    if (r->next()) return r->getInt(1); // first column is the ID
    return 0;
}

// ---------------------------------------------------------
// Function: insertUsersBulk
// Inserts multiple rows efficiently using one prepared statement.
// ---------------------------------------------------------
void insertUsersBulk(sql::Connection* con, const std::vector<User>& users) {
    std::unique_ptr<sql::PreparedStatement> ps(
        con->prepareStatement("INSERT INTO users(name, age) VALUES(?, ?)")
    );

    // Loop through each user and reuse the prepared statement
    for (const auto& u : users) {
        ps->setString(1, u.name);
        if (u.age == 0) ps->setNull(2, 0);
        else ps->setInt(2, u.age);
        ps->executeUpdate();
    }
}

// ---------------------------------------------------------
// Function: updateUserAgeByName
// Updates a user's age by name using a parameterized UPDATE query.
// Returns number of rows affected.
// ---------------------------------------------------------
int updateUserAgeByName(sql::Connection* con, const std::string& name, int newAge) {
    std::unique_ptr<sql::PreparedStatement> ps(
        con->prepareStatement("UPDATE users SET age = ? WHERE name = ?")
    );
    ps->setInt(1, newAge);
    ps->setString(2, name);
    return ps->executeUpdate();
}

// ---------------------------------------------------------
// Function: getUsersByMinAge
// Runs a SELECT query with a parameter and stores results in a vector<User>
// ---------------------------------------------------------
std::vector<User> getUsersByMinAge(sql::Connection* con, int minAge) {
    std::vector<User> out;

    std::unique_ptr<sql::PreparedStatement> ps(
        con->prepareStatement("SELECT id, name, age FROM users WHERE age >= ? ORDER BY age DESC, id ASC")
    );
    ps->setInt(1, minAge);

    std::unique_ptr<sql::ResultSet> rs(ps->executeQuery());
    while (rs->next()) {
        User u{
            rs->getInt("id"),                 // read by column name
            rs->getString("name"),
            rs->isNull("age") ? 0 : rs->getInt("age")
        };
        out.push_back(u);
    }
    return out;
}

// ---------------------------------------------------------
// Function: demoTransaction
// Shows how to group operations in a transaction.
// If one fails, rollback undoes all prior changes.
// ---------------------------------------------------------
void demoTransaction(sql::Connection* con) {
    // Disable autocommit to start a transaction manually
    con->setAutoCommit(false);

    try {
        // Insert a few users
        insertUsersBulk(con, {
            {0, "alice", 24},
            {0, "bob",   29}
            });

        // Update one record
        int changed = updateUserAgeByName(con, "alice", 25);
        std::cout << "Rows updated: " << changed << "\n";

        // Uncomment to simulate an error and trigger rollback:
        // insertUser(con, {0, "alice", 40}); // violates unique constraint

        // Commit changes if all succeeded
        con->commit();
        con->setAutoCommit(true);
        std::cout << "Transaction committed.\n";
    }
    catch (const sql::SQLException& e) {
        // Print the error, rollback changes
        printSqlError(e, "demoTransaction");
        try {
            con->rollback();
            std::cerr << "Transaction rolled back.\n";
        }
        catch (const sql::SQLException& e2) {
            printSqlError(e2, "rollback");
        }
        con->setAutoCommit(true);  // restore default behavior
        throw; // rethrow to inform caller
    }
}

// ---------------------------------------------------------
// Main entry point
// ---------------------------------------------------------
int main() {
    DbConfig cfg; // Use default config values above

    try {
        // Step 1: Get the driver instance (singleton)
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();

        // Step 2: Create a connection to the MySQL server
        std::unique_ptr<sql::Connection> con(driver->connect(cfg.host, cfg.user, cfg.pass));

        // Step 3: Ensure the schema and users table exist
        ensureSchemaAndTables(con.get(), cfg.schema);

        // Step 4: For demo, clear any previous rows (DON’T do this in production)
        {
            std::unique_ptr<sql::Statement> s(con->createStatement());
            s->execute("DELETE FROM users");
        }

        // Step 5: Insert a single record and print its generated ID
        int newId = insertUser(con.get(), { 0, "carol", 32 });
        std::cout << "Inserted carol with id = " << newId << "\n";

        // Step 6: Demonstrate a transaction (insert/update/commit)
        try {
            demoTransaction(con.get());
        }
        catch (...) {
            std::cerr << "Transaction demo failed (rolled back).\n";
        }

        // Step 7: Query all users whose age >= 25
        auto results = getUsersByMinAge(con.get(), 25);
        std::cout << "\nUsers with age >= 25:\n";
        std::cout << std::left << std::setw(5) << "ID" << std::setw(12) << "Name" << "Age\n";
        for (const auto& u : results) {
            std::cout << std::left << std::setw(5) << u.id
                << std::setw(12) << u.name
                << (u.age == 0 ? -1 : u.age) << "\n";
        }

        // Step 8: Update a record again (outside a transaction)
        int affected = updateUserAgeByName(con.get(), "bob", 31);
        std::cout << "\nUpdated rows (bob -> 31): " << affected << "\n";

        // Step 9: Show the final table state
        {
            std::unique_ptr<sql::Statement> s(con->createStatement());
            std::unique_ptr<sql::ResultSet> rs(s->executeQuery("SELECT id, name, age FROM users ORDER BY id"));
            std::cout << "\nFinal users:\n";
            while (rs->next()) {
                std::cout << "ID=" << rs->getInt("id")
                    << " | name=" << rs->getString("name")
                    << " | age=" << (rs->isNull("age") ? -1 : rs->getInt("age"))
                    << "\n";
            }
        }
    }
    catch (const sql::SQLException& e) {
        // Catch database errors (wrong password, bad SQL, etc.)
        printSqlError(e, "main");
        return 1;
    }
    catch (const std::exception& e) {
        // Catch general C++ runtime errors
        std::cerr << "[STD ERROR] " << e.what() << "\n";
        return 1;
    }

    // Program ends successfully
    return 0;
}