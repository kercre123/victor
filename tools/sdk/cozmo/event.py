'''Event dispatch system.

The SDK is based around the dispatch and observation of events.
Objects inheriting from the :class:`Dispatcher` generate and
dispatch events as the state of the robot and its world are updated.

For example the :class:`cozmo.objects.LightCube` class generates an
:class:`~cozmo.objects.EvtObjectTapped` event anytime the cube the object
represents is tapped.

The event can be observed in a number of different ways:

#. By calling the :meth:`~Dispatcher.wait_for` method on the object to observe.
   This will wait until the specific even has been sent to that object and
   return the generated event.
#. By calling :meth:`~Dispatcher.add_event_handler` on the object
   to observe, which will cause the supplied function to be called every time
   the specified event occurs (use the :func:`oneshot` decorator
   to only have the handler called once)
#. By sub-classing a type and implementing a receiver method.
   eg. subclass the :class:`cozmo.objects.LightCube` type and implement `evt_object_tapped`.
   Note that the factory attribute would need to be updated on the
   generating class for your type to be used by the SDK.
   eg. :attr:`~cozmo.world.World.light_cube_factory` in this example.
#. By subclassing a type and implementing a default receiver method.
   Events not dispatched to an explicit receiver method are dispatched to
   `recv_default_handler`.

Events are dispatched to a target object (by calling :meth:`dispatch_event`
on the receiving object).  In line with the above, Upon receiving an event,
the object will:

#. Dispatch the event to any handlers which have explicitly registered interest
   in the event (or a superclass of the event) via
   :meth:`~Dispatcher.add_event_handler` or via :meth:`Dispatcher.wait_for`
#. Dispatch the event ot any "children" of the object (see below)
#. Dispatch the event to method handlers on the receiving object, or the
   `recv_default_handler` if it has no matching handler
#. Dispatch the event to the parent of the object (if any).

Any handler may raise a :class:`~cozmo.exceptions.StopPropogation` exception
to prevent the event reaching any subsequent handlers (but generally should
have no need to).

Child objects receive all events that are sent to the originating object (
which may have multiple children).

Originating objects may have one parent object, which receives all events sent
to its child.

For example, :class:`cozmo.robot.Cozmo` creates a :class:`cozmo.world.World`
object and sets itself as a parent and the World as the child; both receive
events sent to the other.

The World class creates individual :class:`cozmo.objects.ObservableObject` objects
as they are discovered and makes itself a parent, so as to receive all events
sent to the child.  However, it does not make those ObservableObject objects children
for the sake of message dispatch as they only need to receive a small subset
of messages the World object receives.
'''

__all__ = ['Event', 'Dispatcher', 'Filter', 'Handler',
    'oneshot', 'filter_handler', 'wait_for_first']


import asyncio
import collections
import inspect
import re

from . import logger

from . import exceptions
from . import base

# from https://stackoverflow.com/questions/1175208/elegant-python-function-to-convert-camelcase-to-snake-case
_first_cap_re = re.compile('(.)([A-Z][a-z]+)')
_all_cap_re = re.compile('([a-z0-9])([A-Z])')
def _uncamelcase(name):
    s1 = _first_cap_re.sub(r'\1_\2', name)
    return _all_cap_re.sub(r'\1_\2', s1).lower()

registered_events = {}

class _rprop:
    def __init__(self, value):
        self._value = value
    def __get__(self, instance, owner):
        return self._value

class docstr(str):
    @property
    def __doc__(self):
        return self.__str__()


class _AutoRegister(type):
    '''helper to automatically register event classes wherever they're defined
    without requiring a class decorator'''

    def __new__(mcs, name, bases, attrs, **kw):
        if name in ('Event',):
            return super().__new__(mcs, name, bases, attrs, **kw)

        if not (name.startswith('Evt') or name.startswith('_Evt') or name.startswith('_Msg')):
            raise ValueError('Event class names must begin with "Evt (%s)"' % name)

        if '__doc__' not in attrs:
            raise ValueError('Event classes must have a docstring')

        props = set()
        for base in bases:
            if hasattr(base, '_props'):
                props.update(base._props)

        newattrs = {'_internal': False}
        for k, v in attrs.items():
            if k[0] == '_':
                newattrs[k] = v
                continue
            if k in props:
                raise ValueError("Event class %s duplicates property %s defined in superclass" % (self, k))
            props.add(k)
            newattrs[k] = docstr(v)
        newattrs['_props'] = props
        newattrs['_props_sorted'] = sorted(props)

        if name[0] == '_':
            newattrs['_internal'] = True
            name = name[1:]

        # create a read only property for the event name
        newattrs['event_name'] = _rprop(name)
        return super().__new__(mcs, name, bases, newattrs, **kw)

    def __init__(cls, name, bases, attrs, **kw):
        if name in registered_events:
            #for k, v in registered_events[name].__dict__.items():
            #    print(k, v)
            raise ValueError("Duplicate event name %s (%s duplicated by %s)"
                    % (name, _full_qual_name(cls), _full_qual_name(registered_events[name])))
        registered_events[name] = cls
        return super().__init__(name, bases, attrs, **kw)


def _full_qual_name(obj):
    return obj.__module__ + '.' + obj.__qualname__


class Event(metaclass=_AutoRegister):
    """An event representing some action that has occurred.

    Instances of an Event have attributes set to values passed to the event.

    eg. EvtObjectTapped defines obj and tap_count parameters which can
    be accessed as evt.obj and evt.tap_count
    """

    #_first_raised_by = "The object that generated the event"
    #_last_raised_by = "The object that last relayed the event to the dispatched handler"

    def __init__(self, **kwargs):
        unset = self._props.copy()
        for k, v in kwargs.items():
            if k not in self._props:
                raise ValueError("Event %s has no parameter called %s" % (self.event_name, k))
            setattr(self, k, v)
            unset.remove(k)
        for k in unset:
            setattr(self, k, None)
        self._delivered_to = set()

    def __repr__(self):
        kvs = {'name': self.event_name}
        for k in self._props_sorted:
            kvs[k] = getattr(self, k)
        return '<%s %s>' % (self.__class__.__name__, ' '.join(['%s=%s' % kv for kv in kvs.items()]),)

    def _params(self):
        return {k: getattr(self, k) for k in self._props}

    @classmethod
    def _handler_method_name(cls):
        name = 'recv_' + _uncamelcase(cls.event_name)
        if cls._internal:
            name = '_' + name
        return name

    def _dispatch_to_func(self, f):
        return f(self, **self._params())

    def _dispatch_to_obj(self, obj, fallback_to_default=True):
        for cls in self._parent_event_classes():
            f = getattr(obj, cls._handler_method_name(), None)
            if f and not self._is_filtered(f):
                return self._dispatch_to_func(f)

        if fallback_to_default:
            name = 'recv_default_handler'
            if self._internal:
                name = '_' + name
            f = getattr(obj, name, None)
            if f and not self._is_filtered(f):
                return f(self, **self._params())

    def _dispatch_to_future(self, fut):
        if not fut.cancelled():
            fut.set_result(self)

    def _is_filtered(self, f):
        filters = getattr(f, '_handler_filters', None)
        if filters is None:
            return False
        for filter in filters:
            if filter(self):
                return False
        return True

    def _parent_event_classes(self):
        for cls in self.__class__.__mro__:
            if cls != Event and issubclass(cls, Event):
                yield cls


def _register_dynamic_event_type(event_name, attrs):
    return type(event_name, (Event,), attrs)


class Handler(collections.namedtuple('Handler', 'obj evt f')):
    '''A Handler is returned by :func:`Dispatcher.add_event_handler`'''
    __slots__ = ()

    def disable(self):
        '''Removes the handler from the object it was originally registered with.'''
        return self.obj.remove_event_handler(self.evt, self.f)

    @property
    def oneshot(self):
        '''Returns true if the wrapped handler function will only be called once.'''
        return getattr(self.f, '_oneshot_handler', False)


class Dispatcher(base.Base):
    '''Mixin to provide event dispatch handling.'''

    def __init__(self, *a, dispatch_parent=None, loop=None, **kw):
        super().__init__(**kw)
        self._dispatch_parent = dispatch_parent
        self._dispatch_children = []
        self._dispatch_handlers = collections.defaultdict(list)
        if not loop:
            raise ValueError("Loop was not supplied to "+self.__class__.__name__)
        self._loop = loop or asyncio.get_event_loop()
        self._active_dispatch = None

    def _set_parent_dispatcher(self, parent):
        self._dispatch_parent = parent

    def _add_child_dispatcher(self, child):
        self._dispatch_children.append(child)

    def add_event_handler(self, event, f):
        """Register an event handler to be notified when this object receives a type of Event.

        Expects a subclass of Event as the first argument.  If the class has
        subclasses then the handler will be notified for events of that subclass too.
        eg. adding a handler for EvtActionCompleted will cause the handler to
        also be notified for EvtAnimationCompleted as it's a subclass.

        Callable handlers (eg. functions) are called with a first argument containing
        an Event instance and the remaining keyword arguments set as the event parameters.

        eg. ``def my_ontap_handler(evt, *, obj, tap_count, **kwargs)``
        or ``def my_ontap_handler(evt, obj=None, tap_count=None, **kwargs)``

        It's recommended that a **kwargs parameter be included in the definition
        so that future expansion of event parameters do not cause the handler to fail.

        Callbable handlers may raise an events.StopProgation exception to prevent
        other handlers listening to the same event from being triggered.

        :class:`asyncio.Future` handlers are called with a result set to the event.

        Args:
            event (:class:`Event`): A subclass of :class:`Event` (not an instance of that class)
            f (callable): A callable or :class:`asyncio.Future` to execute when the event is received
        Raises:
            :class:`TypeError`: An invalid event type was supplied

        """
        if not issubclass(event, Event):
            raise TypeError("event must be a subclass of Event (not an instance)")

        if isinstance(f, asyncio.Future):
            # futures can only be called once.
            f = oneshot(f)

        handler = Handler(self, event, f)
        self._dispatch_handlers[event.event_name].append(handler)
        return handler

    def remove_event_handler(self, event, f):
        """Remove an event handler for this object.

        Args:
            event (:class:`Event`): The event class, or an instance thereof,
                used with register_event_handler.
            f (callable): The callable object that was passed as a handler to 
                register_event_handler, or a Handler instance
        Raises:
            ValueError: No matching handler found
        """
        if not (isinstance(event, Event) or (isinstance(event, type) and  issubclass(event, Event))):
            raise TypeError("event must be a subclasss or instance of Event")

        if isinstance(f, Handler):
            for i, h in enumerate(self._dispatch_handlers[event.event_name]):
                if h == f:
                    del self._dispatch_handlers[event.event_name][i]
                    return
        else:
            for i, h in enumerate(self._dispatch_handlers[event.event_name]):
                if h.f == f:
                    del self._dispatch_handlers[event.event_name][i]
                    return
        raise ValueError("No matching handler found for %s (%s)" % (event.event_name, f) )

    def dispatch_event(self, event, **kw):
        '''Dispatches a single event to registered handlers.

        Not generally called from user-facing code.

        Args:
            event (:class:`Event): An class or instance of :class:`Event`
            kw (dict): If a class is passed to event, then the remaining keywords
                are passed to it to create an instance of the event.
        Returns:
            A :class:`asyncio.Task` or :class:`asyncio.Future` that will
            complete once all event handlers have been called.
        '''
        event_cls = event
        if not isinstance(event, Event):
            if not isinstance(event, type) or not issubclass(event, Event):
                raise TypeError("events must be a subclass or instance of Event")
            # create an instance of the event if passed a class
            event = event(**kw)
        else:
            event_cls = event.__class__

        event._delivered_to.add(id(self))

        handlers = set()
        futures = []
        events = []
        for cls in event._parent_event_classes():
            handlers.update(self._dispatch_handlers[cls.event_name].copy())
        return asyncio.ensure_future(self._dispatch_event(event, handlers), loop=self._loop)


    async def _dispatch_event(self, event, handlers):
        # iterate through events from child->parent
        # update the dispatched_to set for each event so each handler
        # only receives the most specific event if they are monitoring for both.

        try:
            oneshot = []
            # dispatch to local handlers
            for handler in handlers:
                if event._is_filtered(handler.f):
                    continue

                if isinstance(handler.f, asyncio.Future):
                    event._dispatch_to_future(handler.f)
                else:
                    result = event._dispatch_to_func(handler.f)
                    if asyncio.iscoroutine(result):
                        await result
                if getattr(handler.f, '_oneshot_handler', False):
                    oneshot.append(handler)

            for handler in oneshot:
                # never dispatch an event to this handler again
                handler.disable()
                #self.remove_event_handler(event, handler)

            # dispatch to children
            for child in self._dispatch_children:
                if id(child) not in event._delivered_to:
                    child.dispatch_event(event)

            # dispatch to self methods
            result = event._dispatch_to_obj(self)
            if asyncio.iscoroutine(result):
                await result

            # dispatch to parent dispatcher
            if self._dispatch_parent and id(self._dispatch_parent) not in event._delivered_to:
                self._dispatch_parent.dispatch_event(event)

        except exceptions.StopPropogation:
            pass

    async def wait_for(self, event_or_filter, timeout=30):
        '''Waits for the specified event to be sent to the current object.

        Args:
            event_or_filter (:class:`Event): Either a :class:`Event` class
                or a :class:`Filter` instance to wait to trigger
            timeout: Maximum time to wait for the event.  Pass None to wait indefinitely.
        Returns:
            The :class:`Event` instance that was dispatched
        Raises:
            :class:`asyncio.TimeoutError`
        '''
        f = asyncio.Future(loop=self._loop) # replace with loop.create_future in 3.5.2
        # TODO: add a timer that logs every 5 seconds that the event is still being
        # waited on.  Will help novice programmers realize why their program is hanging.
        f = oneshot(f)

        if isinstance(event_or_filter, Filter):
            f = filter_handler(event_or_filter)(f)
            event = event_or_filter._event
        else:
            event = event_or_filter

        self.add_event_handler(event, f)
        if timeout:
             return await asyncio.wait_for(f, timeout, loop=self._loop)
        return await f



def oneshot(f):
    '''Event handler decorator; causes the handler to only be dispatched to once.'''
    f._oneshot_handler = True
    return f

def filter_handler(event, **filters):
    '''Decorates a handler function or Future to only be called if a filter is matched.

    A handler may apply multiple separate filters; the handlers will be called
    if any of those filters matches.

    For example::

        # Handle only if the majorWin animation completed
        # TODO: fix this; there's no anim_name property (yet)
        @filter_handler(cozmo.anim.EvtAnimationCompleted, anim_name="majorWin")

        # Handle only when the observed object is a LightCube
        @filter_handler(cozmo.objects.EvtObjectObserved, obj=lambda obj: isinstance(cozmo.objects.LightCube))

    Args:
        event (:class:`Event`): The event class to match on
        filter (dict): Zero or more event parameters to filter on.  values may
            be either strings for exact matches, or functions which accept the
            value as the first argument and return a bool indicating whether
            the value passes the filter.
    '''

    if isinstance(event, Filter):
        if len(filters) != 0:
            raise ValueError("Cannot supply filter values when passing a Filter as the first argument")
        filter = event
    else:
        filter = Filter(event, **filters)

    def filter_property(f):
        if hasattr(f, '_handler_filters'):
            f._handler_filters.append(filter)
        else:
            f._handler_filters = [filter]
        return f
    return filter_property



class Filter:
    """A Filter can be passed to Event.wait_for to specify more exactly which events to wait on, beyond event class.

    See the filter_handler method for further details.
    """

    def __init__(self, event, **filters):
        if not issubclass(event, Event):
            raise TypeError("event must be a subclass of Event (not an instance)")
        self._event = event
        self._filters = filters
        for key in self._filters.keys():
            if not hasattr(event, key):
                raise ValueError("Event %s does not define property %s", event.__name__, key)

    def __setattr__(self, key, val):
        if key[0] == '_':
            return super().__setattr__(key, val)
        if not hasattr(self._event, key):
                raise ValueError("Event %s does not define property %s", self.event.__name__, key)
        self._filters[key] = val

    def __call__(self, evt):
        for prop, filter in self._filters.items():
            val = getattr(evt, prop)
            if callable(filter):
                if not filter(val):
                    return False
            elif val != filter:
                return False
        return True


async def wait_for_first(*futures):
    '''Wait the first of a set of events to complete.

    Eg::

        event = cozmo.event.wait_for_first(
            coz.world.wait_for_new_cube(),
            playing_anim.wait_for(cozmo.anim.EvtAnimationCompleted)
        )

    Args:
        futures (list of :class:`asyncio.Future`): The futures or coroutines to wait on.
    '''
    done, pending = asyncio.wait(futures, return_when=asyncio.FIRST_COMPLETED)
    # cancel the pending futures
    for fut in pending:
        fut.cancel()
    return done.pop()
