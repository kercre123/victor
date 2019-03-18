#include "arf/Interprocess.h"
#include "arf/Threads.h"

namespace ARF
{

ExternalCommsRouter *ExternalCommsRouter::_inst = nullptr;

ExternalCommsRouter &ExternalCommsRouter::Inst()
{
    if (!_inst)
    {
        _inst = new ExternalCommsRouter();
    }
    return *_inst;
}

void ExternalCommsRouter::Destruct()
{
    if (_inst)
    {
        delete _inst;
    }
}

ExternalCommsRouter::ExternalCommsRouter()
{
    Lock lock(_mutex);
    _taskTracker = std::make_shared<TaskTracker>();
    Node::RegisterTasks(_taskTracker);
}

ExternalCommsRouter::~ExternalCommsRouter()
{
    Lock lock(_mutex);
    Node::Shutdown();
}

ExternalCommsRouter::TransceiveRegistration::~TransceiveRegistration()
{
    Lock lock(mutex);
    // This takes care of the other NNG resources
    // Note that we don't free lastMsg as NNG takes ownership of it
    nng_close(sock);
}

void ExternalCommsRouter::HandleRxEvent(void *data)
{
    // We're not going to lock here, assuming NNG won't call this callback in multiple threads
    TransceiveRegistration *reg = static_cast<TransceiveRegistration *>(data);
    if (nng_aio_result(reg->aio) != 0)
    {
        std::cerr << "RX failed!" << std::endl;
        return;
    }

    // Retrieve the received message and create a task to deserialize it
    nng_msg *msg = nng_aio_get_msg(reg->aio);
    if (!msg)
    {
        std::cerr << "Didn't get valid message from AIO" << std::endl;
        return;
    }
    Task task = [msg, reg]() {
        reg->rxServiceTask(msg);
    };

    // This is technically a synchronized method, but we expect calls to be fast
    Threadpool::Inst().EnqueueTask(task, TaskPriority::Standard, reg->tracker);

    // Continue receiving
    nng_recv_aio(reg->sock, reg->aio);
}

ExternalCommsRouter::SerializedMessageType ExternalCommsRouter::AllocateMessage(size_t sz)
{
    nng_msg *msg;
    if (nng_msg_alloc(&msg, sz) != 0)
    {
        return nullptr;
    }
    return msg;
}

void *ExternalCommsRouter::GetMessageBody(SerializedMessageType msg)
{
    return (msg) ? nng_msg_body(msg) : nullptr;
}

size_t ExternalCommsRouter::GetMessageSize(SerializedMessageType msg)
{
    return (msg) ? nng_msg_len(msg) : 0;
}

bool ExternalCommsRouter::SetupRx(const std::string &url, TransceiveRegistration &reg)
{
    // subscribe to everything (empty means all topics)
    CHECK_NNG_RV(nng_sub0_open(&reg.sock), ("sub0_open failed: " + url));
    CHECK_NNG_RV(nng_setopt_size(reg.sock, NNG_OPT_RECVMAXSZ, 1024 * 1024 * 5), "set recvmaxsz failed");
    CHECK_NNG_RV(nng_setopt(reg.sock, NNG_OPT_SUB_SUBSCRIBE, "", 0), "set sub failed");

    // Create dialer to wait on corresponding transmitter
    // Set min and max poll times
    CHECK_NNG_RV(nng_dialer_create(&reg.dialer, reg.sock, url.c_str()), "create dialer failed");
    CHECK_NNG_RV(nng_dialer_setopt_ms(reg.dialer, NNG_OPT_RECONNMINT, 1e2), "set reconnmint failed");
    CHECK_NNG_RV(nng_dialer_setopt_ms(reg.dialer, NNG_OPT_RECONNMAXT, 1e4), "set reconnmaxt failed");
    CHECK_NNG_RV(nng_dialer_start(reg.dialer, NNG_FLAG_NONBLOCK), "start dialer failed");
    return true;
}

bool ExternalCommsRouter::SetupTx(const std::string &url, TransceiveRegistration &reg)
{
    CHECK_NNG_RV(nng_pub0_open(&reg.sock), ("pub0_open failed: " + url));

    // Create listener to handle connection requests
    CHECK_NNG_RV(nng_listener_create(&reg.listener, reg.sock, url.c_str()), "create listener failed");
    CHECK_NNG_RV(nng_listener_start(reg.listener, NNG_FLAG_NONBLOCK), "start listener failed");
    return true;
}

} // end namespace ARF
