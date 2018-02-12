const Cast = require('../util/cast');

class Scratch3EventBlocks {
    constructor (runtime) {
        /**
         * The runtime instantiating this block package.
         * @type {Runtime}
         */
        this.runtime = runtime;
    }

    /**
     * Retrieve the block primitives implemented by this package.
     * @return {object.<string, Function>} Mapping of opcode to Function.
     */
    getPrimitives () {
        return {
            event_broadcast: this.broadcast, // *** ANKI CHANGE *** DEPRECATED. This is old implementation.
            event_broadcast_with_broadcast_input: this.broadcastWithBroadcastInput, // *** ANKI CHANGE *** Replaces event_broadcast with new Scratch event
            event_broadcastandwait: this.broadcastAndWait, // *** ANKI CHANGE *** DEPRECATED. This is old implementation.
            event_broadcastandwait_with_broadcast_input: this.broadcastAndWaitWithBroadcastInput, // *** ANKI CHANGE *** Replaces event_broadcast with new Scratch event
            event_whengreaterthan: this.hatGreaterThanPredicate
        };
    }

    getHats () {
        return {
            event_whenflagclicked: {
                restartExistingThreads: true
            },
            event_whenkeypressed: {
                restartExistingThreads: false
            },
            event_whenthisspriteclicked: {
                restartExistingThreads: true
            },
            event_whenbackdropswitchesto: {
                restartExistingThreads: true
            },
            event_whengreaterthan: {
                restartExistingThreads: false,
                edgeActivated: true
            },
            event_whenbroadcastreceived: { // *** ANKI CHANGE *** DEPRECATED. This is old implementation.
                restartExistingThreads: true
            },
            event_whenbroadcastreceived_with_broadcast_input: { // *** ANKI CHANGE ***  Replaces event_whenbroadcastreceived with new Scratch event
                restartExistingThreads: true
            },

            // *** ANKI CHANGE ***
            // Add Cozmo-specific hat blocks
            cozmo_event_on_face: {
                restartExistingThreads: false
            },
            cozmo_event_on_happy_face: {
                restartExistingThreads: false
            },
            cozmo_event_on_sad_face: {
                restartExistingThreads: false
            },
            cozmo_event_on_see_cube: {
                restartExistingThreads: false
            },
            cozmo_event_on_cube_tap: {
                restartExistingThreads: false
            },
            cozmo_event_on_cube_moved: {
                restartExistingThreads: false
            }
        };
    }

    hatGreaterThanPredicate (args, util) {
        const option = Cast.toString(args.WHENGREATERTHANMENU).toLowerCase();
        const value = Cast.toNumber(args.VALUE);
        // @todo: Other cases :)
        if (option === 'timer') {
            return util.ioQuery('clock', 'projectTimer') > value;
        }
        return false;
    }

    // *** ANKI CHANGE ***
    // Deprecated
    // This is an old implementation.
    broadcast (args, util) {
        const broadcastOption = Cast.toString(args.BROADCAST_OPTION);
        util.startHats('event_whenbroadcastreceived', {
            BROADCAST_OPTION: broadcastOption
        });
    }

    // *** ANKI CHANGE ***
    // Replaces broadcast with new Scratch event
    broadcastWithBroadcastInput (args, util) {
        const broadcastVar = util.runtime.getTargetForStage().lookupBroadcastMsg(
            args.BROADCAST_OPTION.id, args.BROADCAST_OPTION.name);
        if (broadcastVar) {
            const broadcastOption = broadcastVar.name;
            util.startHats('event_whenbroadcastreceived_with_broadcast_input', {
                BROADCAST_OPTION: broadcastOption
            });
        }
    }

    // *** ANKI CHANGE *** 
    // Deprecated
    // This is an old implementation.
    broadcastAndWait (args, util) {
        const broadcastOption = Cast.toString(args.BROADCAST_OPTION);
        // Have we run before, starting threads?
        if (!util.stackFrame.startedThreads) {
            // No - start hats for this broadcast.
            util.stackFrame.startedThreads = util.startHats(
                'event_whenbroadcastreceived', {
                    BROADCAST_OPTION: broadcastOption
                }
            );
            if (util.stackFrame.startedThreads.length === 0) {
                // Nothing was started.
                return;
            }
        }
        // We've run before; check if the wait is still going on.
        const instance = this;
        const waiting = util.stackFrame.startedThreads.some(thread => instance.runtime.isActiveThread(thread));
        if (waiting) {
            util.yield();
        }
    }

    // *** ANKI CHANGE ***
    // Replaces broadcastAndWait with new Scratch event
    broadcastAndWaitWithBroadcastInput (args, util) {
        const broadcastVar = util.runtime.getTargetForStage().lookupBroadcastMsg(
            args.BROADCAST_OPTION.id, args.BROADCAST_OPTION.name);
        if (broadcastVar) {
            const broadcastOption = broadcastVar.name;
            // Have we run before, starting threads?
            if (!util.stackFrame.startedThreads) {
                // No - start hats for this broadcast.
                util.stackFrame.startedThreads = util.startHats(
                    'event_whenbroadcastreceived_with_broadcast_input', {
                        BROADCAST_OPTION: broadcastOption
                    }
                );
                if (util.stackFrame.startedThreads.length === 0) {
                    // Nothing was started.
                    return;
                }
            }
            // We've run before; check if the wait is still going on.
            const instance = this;
            const waiting = util.stackFrame.startedThreads.some(thread => instance.runtime.isActiveThread(thread));
            if (waiting) {
                util.yield();
            }
        }
    }
}

module.exports = Scratch3EventBlocks;
