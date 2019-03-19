#include "arf/Topics.h"

namespace ARF
{

size_t TopicRegistration::NumInputs() const
{
    Lock lock( _mutex );
    return _inputs.size();
}

size_t TopicRegistration::NumOutputs() const
{
    Lock lock( _mutex );
    return _outputs.size();
}


TopicDirectory* TopicDirectory::_inst = new TopicDirectory();

TopicDirectory::TopicDirectory() {}

TopicDirectory::~TopicDirectory()
{
    // TODO cleanup?
}

TopicDirectory& TopicDirectory::Inst()
{
    return *_inst;
}

void TopicDirectory::Destruct()
{
    if( _inst )
    {
        delete _inst;
    }
}

void TopicDirectory::Clear()
{
    _topicRegistry.clear();
}

size_t TopicDirectory::NumTopics() const
{
    Lock lock( _mutex );
    return _topicRegistry.size();
}

TopicRegistration::ConstPtr TopicDirectory::GetTopic( const std::string& name ) const
{
    Lock lock( _mutex );
    TopicRegistry::const_iterator item = _topicRegistry.find( name );
    if( item == _topicRegistry.end() ) { return nullptr; }
    return item->second; 
}



} // end namespace arf
