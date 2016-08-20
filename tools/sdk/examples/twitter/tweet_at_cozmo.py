import cozmo
import twitter_helpers
import cozmo_twitter_keys as twitter_keys
from cozmo.util import degrees


'''Example for integrating Cozmo with Twitter
   Lets you tweet at your Cozmo to control the robot
   See cozmo_twitter_keys.py for details on how to setup a twitter account for your Cozmo and get access keys
'''


def extract_float(args, index=0):
    if len(args) > index:
        try:
            float_val = float(args[index])
            return float_val
        except ValueError:
            pass
    return None


def extract_next_float(args, index=0):
    for i in range(index, len(args)):
        try:
            float_val = float(args[index])
            return float_val, i
        except ValueError:
            pass
    return None, None


class ReactToTweetsStreamListener(twitter_helpers.CozmoTweetStreamListener):
    '''React to Tweets sent to our Cozmo, live, as they happen...'''

    def __init__(self, coz, twitter_api):
        super().__init__(coz, twitter_api)


    # Useful during development - an easy way to delete all of Cozmo's tweets
    # def do_deleteall(self, args):
    #     cozmo.logger.info('Deleting all of your tweets')
    #     twitter_helpers.delete_all_tweets(self.twitter_api)
    #     return None


    def do_drive(self, args):
        """drive X"""
        usage = "'drive X' where X is number of seconds to drive for"
        error_message = ""

        drive_duration = extract_float(args)

        if drive_duration is not None:
            drive_speed = 50
            drive_dir = "forwards"
            if (drive_duration < 0):
                drive_speed = -drive_speed
                drive_duration = -drive_duration
                drive_dir = "backwards"

            self.cozmo.drive_wheels(drive_speed, drive_speed, duration=drive_duration)
            return "I drove " + drive_dir + " for " + str(drive_duration) + " seconds!"
            # except Exception as e:
            #     error_message = " Exception: " + str(e)

        return "Error: usage = " + usage + error_message


    def do_turn(self, args):

        usage = "'turn X' where X is a number of degrees to turn"

        drive_angle = extract_float(args)

        if drive_angle is not None:
            self.cozmo.turn_in_place(degrees(drive_angle)).wait_for_completed()
            return "I turned " + str(drive_angle) + " degrees!"

        return "Error: usage = " + usage


    def do_lift(self, args):

        usage = "'lift X' where X is speed to move lift"

        lift_velocity = extract_float(args)

        if lift_velocity is not None:
            self.cozmo.move_lift(lift_velocity)#.wait_for_completed()
            return "I moved lift " + str(lift_velocity) + " vel!"

        return "Error: usage = " + usage


    def do_tilthead(self, args):

        usage = "'tilthead X' where X is speed to tilt head"

        head_velocity = extract_float(args)

        if head_velocity is not None:
            self.cozmo.move_head(head_velocity)#.wait_for_completed()
            return "I moved head " + str(head_velocity) + " vel!"

        return "Error: usage = " + usage


    def do_say(self, args):

        usage = "'say X' where X is any text for cozmo to say"

        entire_message = None
        if len(args) > 0:
            try:
                entire_message = ""
                for s in args:
                    entire_message = entire_message + " " + str(s)
                entire_message = entire_message.strip()
            except:
                pass

        if (entire_message is not None) and (len(entire_message) > 0):
            self.cozmo.say_text(entire_message).wait_for_completed()
            return 'I said "' + entire_message + '"!'

        return "Error: usage = " + usage


    # not yet implemented
    #def do_photo(self, args):


    def get_supported_commands(self):
        '''Construct a list of all methods in this class that start with "do_" - these are commands we accept'''
        prefix_str = "do_"
        prefix_len = len(prefix_str)
        supported_commands = []
        for func_name in dir(self.__class__):
            if func_name.startswith(prefix_str):
                supported_commands.append( func_name[prefix_len:] )
        return supported_commands


    def extract_command_from_string(self, in_string):
        '''Separate inString at each space, loop through until we find a command, return tuple of command and args'''

        split_string = in_string.split()

        for i in range(len(split_string)):
            try:
                func = getattr(self, 'do_' + split_string[i].lower())
            except AttributeError:
                func = None

            if func:
                command_args = split_string[i + 1:]
                return func, command_args

        # No valid command found
        return None, None


    def on_tweet_from_user(self, json_data, tweet_text, from_user, is_retweet):
        '''Handle every new tweet as it appears'''

        # ignore retweets
        if is_retweet:
            return True

        # ignore any replies from this account (otherwise we'd infinite loop as soon as we reply)
        # allow other messages from this account (so you can tweet at yourself to control Cozmo if you want)

        user_me = self.twitter_api.me()
        is_from_me = (from_user.get('id') == user_me.id)

        if is_from_me and tweet_text.startswith("@"):
            # ignore replies from this account
            return

        from_user_name = from_user.get('screen_name')

        tweet_id = json_data.get('id_str')

        command_func, command_args = self.extract_command_from_string(tweet_text)

        reply_prefix = "@" + from_user_name + " "
        if command_func is not None:
            result_string = command_func(command_args)
            if result_string:
                self.post_tweet(reply_prefix + result_string, tweet_id)
        else:
            self.post_tweet(reply_prefix + "Sorry I don't understand, available commands are: "
                            + str(self.get_supported_commands()), tweet_id)


def run(coz_conn):
    coz = coz_conn.wait_for_robot()

    twitter_api, twitter_auth = twitter_helpers.init_twitter(twitter_keys)
    stream_listener = ReactToTweetsStreamListener(coz, twitter_api)
    twitter_stream = twitter_helpers.CozmoStream(twitter_auth, stream_listener)
    twitter_stream.userstream(_with='user')


if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)

