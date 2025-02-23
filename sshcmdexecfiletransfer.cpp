#include "sshcmdexecfiletransfer.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

SSHCmdExecFileTransfer::SSHCmdExecFileTransfer() : session(nullptr), connected(false) {}

SSHCmdExecFileTransfer::~SSHCmdExecFileTransfer() {
    disconnect();
}

void SSHCmdExecFileTransfer::disconnect() {
    if (session) {
        if (connected) {
            ssh_disconnect(session);
        }
        ssh_free(session);
        session = nullptr;
        connected = false;
    }
}

void SSHCmdExecFileTransfer::set_error(const std::string& error) {
    last_error = error;
    std::cerr << "SSH Error: " << error << std::endl;
}

bool SSHCmdExecFileTransfer::connect_session(const char* host, const char* user, const char* password) {
    if (is_connected()) {
        disconnect();
    }

    session = ssh_new();
    if (session == nullptr) {
        set_error("Failed to create SSH session");
        return false;
    }

    int verbosity = SSH_LOG_NOLOG;
    int port = 22;

    ssh_options_set(session, SSH_OPTIONS_HOST, host);
    ssh_options_set(session, SSH_OPTIONS_USER, user);
    ssh_options_set(session, SSH_OPTIONS_PORT, &port);
    ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);

    // Set connection timeout to 10 seconds
    long timeout = 10;
    ssh_options_set(session, SSH_OPTIONS_TIMEOUT, &timeout);

    int rc = ssh_connect(session);
    if (rc != SSH_OK) {
        set_error("Connection error: " + std::string(ssh_get_error(session)));
        ssh_free(session);
        session = nullptr;
        return false;
    }

    rc = ssh_userauth_password(session, nullptr, password);
    if (rc != SSH_AUTH_SUCCESS) {
        set_error("Authentication failed: " + std::string(ssh_get_error(session)));
        ssh_disconnect(session);
        ssh_free(session);
        session = nullptr;
        return false;
    }

    connected = true;
    return true;
}

SSHCmdExecFileTransfer::ConnectionResult SSHCmdExecFileTransfer::test_connection(
    const char* host, const char* user, const char* password) {

    ConnectionResult result;
    result.success = false;
    result.error_code = -1;

    try {
        if (connect_session(host, user, password)) {
            result.success = true;
            result.error_message = "Connection successful";
            result.error_code = 0;
            disconnect();
        } else {
            result.error_message = get_last_error();
            result.error_code = 1;
        }
    } catch (const std::exception& e) {
        result.error_message = "Exception during connection test: " + std::string(e.what());
        result.error_code = 2;
    }

    return result;
}

SSHCmdExecFileTransfer::CommandResult SSHCmdExecFileTransfer::execute_remote_command(
    const char* host, const char* user, const char* password, const char* command) {

    CommandResult result;
    result.output = "";
    result.exit_status = -1;
    result.success = false;
    result.error_message = "";

    try {
        if (!connect_session(host, user, password)) {
            result.error_message = get_last_error();
            return result;
        }

        ssh_channel channel = ssh_channel_new(session);
        if (channel == nullptr) {
            result.error_message = "Failed to create channel: " + std::string(ssh_get_error(session));
            return result;
        }

        if (ssh_channel_open_session(channel) != SSH_OK) {
            result.error_message = "Error opening channel: " + std::string(ssh_get_error(session));
            ssh_channel_free(channel);
            return result;
        }

        if (ssh_channel_request_exec(channel, command) != SSH_OK) {
            result.error_message = "Error executing command: " + std::string(ssh_get_error(session));
            ssh_channel_close(channel);
            ssh_channel_free(channel);
            return result;
        }

        char buffer[256];
        int nbytes;

        // Read stdout
        while ((nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0)) > 0) {
            result.output.append(buffer, nbytes);
        }

        // Read stderr
        while ((nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 1)) > 0) {
            result.error_message.append(buffer, nbytes);
        }

        result.exit_status = ssh_channel_get_exit_status(channel);
        result.success = (result.exit_status == 0);

        ssh_channel_send_eof(channel);
        ssh_channel_close(channel);
        ssh_channel_free(channel);

    } catch (const std::exception& e) {
        result.error_message = "Exception during command execution: " + std::string(e.what());
        result.success = false;
    }

    return result;
}

SSHCmdExecFileTransfer::FileTransferResult SSHCmdExecFileTransfer::upload_file(
    const char* host, const char* user, const char* password,
    const char* local_file, const char* remote_file) {

    FileTransferResult result;
    result.success = false;
    result.bytes_transferred = 0;
    result.error_message = "";

    try {
        if (!connect_session(host, user, password)) {
            result.error_message = get_last_error();
            return result;
        }

        sftp_session sftp = sftp_new(session);
        if (sftp == nullptr) {
            result.error_message = ssh_get_error(session);
            return result;
        }

        if (sftp_init(sftp) != SSH_OK) {
            result.error_message = ssh_get_error(session);
            sftp_free(sftp);
            return result;
        }

        FILE* local = fopen(local_file, "rb");
        if (local == nullptr) {
            result.error_message = "Error opening local file: " + std::string(strerror(errno));
            sftp_free(sftp);
            return result;
        }

        sftp_file remote = sftp_open(sftp, remote_file,
                                     O_WRONLY | O_CREAT | O_TRUNC,
                                     S_IRWXU);
        if (remote == nullptr) {
            result.error_message = ssh_get_error(session);
            fclose(local);
            sftp_free(sftp);
            return result;
        }

        char buffer[16384];
        size_t nbytes;

        while ((nbytes = fread(buffer, 1, sizeof(buffer), local)) > 0) {
            ssize_t nwritten = sftp_write(remote, buffer, nbytes);
            if (nwritten != nbytes) {
                result.error_message = ssh_get_error(session);
                sftp_close(remote);
                fclose(local);
                sftp_free(sftp);
                return result;
            }
            result.bytes_transferred += nwritten;
        }

        sftp_close(remote);
        fclose(local);
        sftp_free(sftp);

        result.success = true;

    } catch (const std::exception& e) {
        result.error_message = "Exception during file transfer: " + std::string(e.what());
        return result;
    }

    return result;
}
