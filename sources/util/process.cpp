#include "util/process.h"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>

#include <array>

namespace gnome_pkpass::util {

bool command_exists(const std::string &s_program) {
    if (s_program.empty()) {
        return false;
    }
    if (s_program.find('/') != std::string::npos) {
        return 0 == ::access(s_program.c_str(), X_OK);
    }

    const char *s_path = ::getenv("PATH");
    if (nullptr == s_path) {
        return false;
    }
    std::string s_rest(s_path);
    while (!s_rest.empty()) {
        const std::size_t u_colon = s_rest.find(':');
        const std::string s_directory =
            (std::string::npos == u_colon) ? s_rest : s_rest.substr(0, u_colon);
        if (!s_directory.empty()) {
            const std::string s_candidate = s_directory + "/" + s_program;
            if (0 == ::access(s_candidate.c_str(), X_OK)) {
                return true;
            }
        }
        if (std::string::npos == u_colon) {
            break;
        }
        s_rest = s_rest.substr(u_colon + 1);
    }
    return false;
}

bool run_command(const std::vector<std::string> &o_arguments, std::string &s_output,
                 int &i_status, bool &b_exited, std::string &s_error) {
    s_output.clear();
    i_status = -1;
    b_exited = false;

    if (o_arguments.empty()) {
        s_error = "no program was named";
        return false;
    }

    int o_pipe[2];
    if (0 != ::pipe(o_pipe)) {
        s_error = std::string("cannot create a pipe: ") + std::strerror(errno);
        return false;
    }

    const pid_t i_child = ::fork();
    if (0 > i_child) {
        ::close(o_pipe[0]);
        ::close(o_pipe[1]);
        s_error = std::string("cannot start a child process: ") + std::strerror(errno);
        return false;
    }

    if (0 == i_child) {
        ::close(o_pipe[0]);
        if (STDOUT_FILENO != ::dup2(o_pipe[1], STDOUT_FILENO) ||
            STDERR_FILENO != ::dup2(o_pipe[1], STDERR_FILENO)) {
            ::_exit(127);
        }
        ::close(o_pipe[1]);

        std::vector<char *> o_raw;
        o_raw.reserve(o_arguments.size() + 1);
        for (const std::string &s_argument : o_arguments) {
            o_raw.push_back(const_cast<char *>(s_argument.c_str()));
        }
        o_raw.push_back(nullptr);
        ::execvp(o_raw[0], o_raw.data());
        ::_exit(127);
    }

    ::close(o_pipe[1]);
    std::array<char, 4096> o_buffer{};
    for (;;) {
        const ssize_t i_read = ::read(o_pipe[0], o_buffer.data(), o_buffer.size());
        if (0 > i_read) {
            if (EINTR == errno) {
                continue;
            }
            break;
        }
        if (0 == i_read) {
            break;
        }
        s_output.append(o_buffer.data(), static_cast<std::size_t>(i_read));
    }
    ::close(o_pipe[0]);

    int i_wait_status = 0;
    while (0 > ::waitpid(i_child, &i_wait_status, 0)) {
        if (EINTR != errno) {
            s_error = std::string("cannot collect the child process: ") + std::strerror(errno);
            return false;
        }
    }

    if (WIFEXITED(i_wait_status)) {
        b_exited = true;
        i_status = WEXITSTATUS(i_wait_status);
        return true;
    }
    if (WIFSIGNALED(i_wait_status)) {
        s_error = "the program was ended by a signal";
        return false;
    }
    s_error = "the program ended in an unexpected way";
    return false;
}

}  // namespace gnome_pkpass::util
