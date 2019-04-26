// Copyright Splunk, Inc. 2014
//
// Author: Gareth Watts <gareth@splunk.com>

package iniconf

// Types implementing the RegisterSectionObserver interface may be passed to RegisterGlobalObserver
// or RegisterSectionObserver to be advised of new sections being created.
type NewSectionObserver interface {
	IniSectionAdded(i *IniConf, sectionName string)
}

// Types implementing the DeleteSectionObserver interface may be passed to RegisterGlobalObserver
// or RegisterSectionObserver to be advised of sections being deleted.
type DeleteSectionObserver interface {
	IniSectionDeleted(i *IniConf, sectionName string)
}

// Types implementing the EntryChangeObserver interface may be passed to RegisterGlobalObserver
// or RegisterSectionObserver to be advised of entries within sections being
// added, removed or updated.
//
// New entries will have an empty oldValue.
// Deleted entries will have an empty newValue.
type EntryChangeObserver interface {
	IniEntryChanged(i *IniConf, sectionName, entryName, oldValue, newValue string)
}

type entryChange struct {
	sectionName string
	entryName   string
	oldValue    string
	newValue    string
}

func (i *IniConf) notify(s *section, f func(obv interface{})) {
	i.mutex.RLock()
	if i.observersDisabled {
		i.mutex.RUnlock()
		return
	}
	// make a copy of the observers so that observers
	// may call Register/Unregister without racing
	globalObv := make([]interface{}, len(i.globalObservers))
	var sectionObv []interface{}

	for _, obv := range i.globalObservers {
		globalObv = append(globalObv, obv)
	}
	if s != nil {
		sidxname := i.idxName(s.name)
		sectionObv = make([]interface{}, len(i.sectionObservers[sidxname]))
		for _, obv := range i.sectionObservers[sidxname] {
			sectionObv = append(sectionObv, obv)
		}
	}
	i.mutex.RUnlock()

	for _, obv := range globalObv {
		f(obv)
	}
	if s != nil {
		for _, obv := range sectionObv {
			f(obv)
		}
	}
}

func (i *IniConf) notifyNewSection(s *section) {
	i.notify(s, func(obv interface{}) {
		if obv, ok := obv.(NewSectionObserver); ok {
			obv.IniSectionAdded(i, s.name)
		}
	})
}

func (i *IniConf) notifyDeleteSection(s *section) {
	i.notify(s, func(obv interface{}) {
		if obv, ok := obv.(DeleteSectionObserver); ok {
			obv.IniSectionDeleted(i, s.name)
		}
	})
}

func (i *IniConf) notifyEntriesChanged(s *section, chlist []entryChange) {
	i.notify(s, func(obv interface{}) {
		if obv, ok := obv.(EntryChangeObserver); ok {
			for _, ch := range chlist {
				obv.IniEntryChanged(i, ch.sectionName, ch.entryName,
					ch.oldValue, ch.newValue)
			}
		}
	})
}

func (i *IniConf) notifyEntryChanged(s *section, ch entryChange) {
	i.notify(s, func(obv interface{}) {
		if obv, ok := obv.(EntryChangeObserver); ok {
			obv.IniEntryChanged(i, ch.sectionName, ch.entryName, ch.oldValue, ch.newValue)
		}
	})
}

// RegisterGlobalRegister registers an observer to be notified of all section
// and/or entry updates.
//
// The observer must implement one or more of the NewSectionObserver,
// DeleteSectionObserver and EntryChangeObserver interfaces.
// Additionally you should pass a pointer to that observer to this method
// else UnregisterObserver will fail to remove the observer.
func (i *IniConf) RegisterGlobalObserver(ob interface{}) {
	// Check that the interface implements at least one valid observor interface
	switch ob.(type) {
	case NewSectionObserver:
	case DeleteSectionObserver:
	case EntryChangeObserver:
	default:
		panic(ErrBadObserver)
	}
	i.mutex.Lock()
	defer i.mutex.Unlock()
	i.globalObservers[ob] = ob
}

// RegisterSectionObserver registers an observer to be notified of changes
// to a named section,  whether or not it exists yet.
//
// The observer must implement one or more of the NewSectionObserver,
// DeleteSectionObserver and EntryChangeObserver interfaces.
// Additionally you should pass a pointer to that observer to this method
// else UnregisterObserver will fail to remove the observer.
func (i *IniConf) RegisterSectionObserver(sectionName string, ob interface{}) {
	switch ob.(type) {
	case NewSectionObserver:
	case DeleteSectionObserver:
	case EntryChangeObserver:
	default:
		panic(ErrBadObserver)
	}
	i.mutex.Lock()
	defer i.mutex.Unlock()

	if i.sectionObservers[sectionName] == nil {
		i.sectionObservers[sectionName] = make(map[interface{}]interface{})
	}
	i.sectionObservers[sectionName][ob] = ob
}

// UnregisterGlobalObserver unregisters a previously registered global observer.
func (i *IniConf) UnregisterGlobalObserver(ob interface{}) {
	i.mutex.Lock()
	defer i.mutex.Unlock()
	if _, present := i.globalObservers[ob]; present {
		delete(i.globalObservers, ob)
	}
}

// UnregisterSectionObserver unregisters a previously registered section observer.
func (i *IniConf) UnregisterSectionObserver(sectionName string, ob interface{}) {
	i.mutex.Lock()
	defer i.mutex.Unlock()
	observers := i.sectionObservers[sectionName]
	if observers == nil {
		return
	}
	if _, present := observers[ob]; present {
		delete(observers, ob)
	}
}

// DisableObservers temporarily disables observer notifications.
// Any notifications that would of been generated after this call has been
// made will be discarded.
func (i *IniConf) DisableObservers() {
	i.mutex.Lock()
	defer i.mutex.Unlock()
	i.observersDisabled = true
}

// EnableObservers re-enables all observers.
func (i *IniConf) EnableObservers() {
	i.mutex.Lock()
	defer i.mutex.Unlock()
	i.observersDisabled = false
}
