#ifndef SSH_CMD_EXEC_FILE_TRANSFER_H
#define SSH_CMD_EXEC_FILE_TRANSFER_H

#include <iostream>
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <string>
#include <stdexcept>

class SSHCmdExecFileTransfer {
public:
    // struct is a user defined data type
    struct CommandResult {
        std::string output;
        int exit_status;
        bool success;
        std::string error_message;
    };

    struct FileTransferResult {
        bool success;
        size_t bytes_transferred;
        std::string error_message;
    };

    struct ConnectionResult {
        bool success;
        std::string error_message;
        int error_code;
    };

    SSHCmdExecFileTransfer();
    ~SSHCmdExecFileTransfer();

    ConnectionResult test_connection(const char* host, const char* user, const char* password);

    bool is_connected() const { return session != nullptr && connected; }

    std::string get_last_error() const { return last_error; }

    CommandResult execute_remote_command(const char* host, const char* user,
                                         const char* password, const char* command);

    FileTransferResult upload_file(const char* host, const char* user,
                                   const char* password, const char* local_file,
                                   const char* remote_file);

    void disconnect();

private:
    ssh_session session;
    bool connected;
    std::string last_error;

    bool connect_session(const char* host, const char* user, const char* password);
    void set_error(const std::string& error);
};

#endif // SSH_CMD_EXEC_FILE_TRANSFER_H
