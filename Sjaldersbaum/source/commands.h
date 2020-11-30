#pragma once

#include <string>
#include <queue>

#include "units.h"

/*------------------------------------------------------------------------------------------------*/

/*
Command syntax: "command_name(optional_args)", where:
    1) command_name corresponds to some event/function (see source);
    2) optional_args can be converted to that function's required arguments.

Note that optional_args may be surrounded by double-quotes, in which case:
    a) everything between the quotes is ignored by the command_sequence executor; (this allows us
       to use parentheses and semicolons in arguments without conflicting the executor).
    b) the quotes will be removed before being passed on the the function.
*/
class Executor
{
public:
    static Executor& instance();

    void update(Seconds elapsed_time);

    // Pushes a sequence of commands, separated by semicolons, into a queue.
    // Postponing halts the execution of all proceeding commands. A postpone can also
    // be inserted in the middle of the sequence by using the 'postpone(sec);' command.
    void queue_execution(const std::string& command_sequence, Seconds postpone = 0.f);

    void queue_execution(const std::vector<std::string>& command_sequence_list, Seconds postpone = 0.f);

    // Returns true if any commands are queued/postponed for execution.
    bool is_busy() const;

    // Returns all commands currently in the queue and removes them from it.
    std::queue<std::string> extract_queue();

private:
    void execute(const std::string command);

private:
    std::queue<std::string> commands;
    Seconds postpone_timer;

private:
    Executor() : postpone_timer{ 0.f } {};
    Executor(const Executor&) = delete;
    Executor(Executor&&) = delete;
    Executor& operator=(const Executor&) = delete;
    Executor& operator=(Executor&&) = delete;
};