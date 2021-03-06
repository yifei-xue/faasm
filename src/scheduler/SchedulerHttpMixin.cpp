#include "SchedulerHttpMixin.h"
#include "Scheduler.h"

#include <util/logging.h>
#include <util/timing.h>

namespace scheduler {
    std::string SchedulerHttpMixin::executeFunction(message::Message &msg) {
        const std::shared_ptr<spdlog::logger> &logger = util::getLogger();
        util::SystemConfig &conf = util::getSystemConfig();

        if (msg.user().empty()) {
            return "Empty user";
        } else if (msg.function().empty()) {
            return "Empty function";
        }

        util::setMessageId(msg);

        auto tid = (pid_t) syscall(SYS_gettid);

        const std::string funcStr = util::funcToString(msg, true);
        logger->debug("Worker HTTP thread {} scheduling {}", tid, funcStr);

        // Schedule it
        scheduler::Scheduler &sch = scheduler::getScheduler();
        sch.callFunction(msg);

        // Await result on global bus (may have been executed on a different worker)
        if (msg.isasync()) {
            return util::buildAsyncResponse(msg);
        } else {
            logger->debug("Worker thread {} awaiting {}", tid, funcStr);

            try {
                scheduler::GlobalMessageBus &globalBus = scheduler::getGlobalMessageBus();
                const message::Message result = globalBus.getFunctionResult(msg.id(), conf.globalMessageTimeout);
                logger->debug("Worker thread {} result {}", tid, funcStr);

                return result.outputdata() + "\n";
            } catch (redis::RedisNoResponseException &ex) {
                return "No response from function\n";
            }
        }
    }
}
