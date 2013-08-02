function ControlCozmo(this)
%Check the keyboard keys and issue robot commands

DEBUG = false;

% desktop;
% keyboard;

key = wb_robot_keyboard_get_key();

% Should we lock to a block which is close to the connector?
if this.locked == false && ...
        wb_connector_get_presence(this.con_lift) == 1
    
    if this.unlockhysteresis == 0
        
        wb_connector_lock(this.con_lift);
        this.locked = true;
        fprintf('LOCKED!\n');
    else
        this.unlockhysteresis = this.unlockhysteresis - 1;
        
    end
    
end

switch (key)
    
    case WB_ROBOT_KEYBOARD_UP
        if DEBUG
            fprintf('Key Pressed: UP\n');
        end
        
        this.SetAngularWheelVelocity( ...
            this.DRIVE_VELOCITY_SLOW, this.DRIVE_VELOCITY_SLOW);
        
    case WB_ROBOT_KEYBOARD_DOWN
        if DEBUG
            fprintf('Key Pressed: DOWN\n');
        end
        
        this.SetAngularWheelVelocity( ...
            -this.DRIVE_VELOCITY_SLOW, -this.DRIVE_VELOCITY_SLOW);
        
    case WB_ROBOT_KEYBOARD_LEFT
        if DEBUG
            fprintf('Key Pressed: LEFT\n');
        end
        
        this.SetAngularWheelVelocity( ...
            -this.TURN_VELOCITY_SLOW, this.TURN_VELOCITY_SLOW);
        
    case WB_ROBOT_KEYBOARD_RIGHT
        if DEBUG
            fprintf('Key Pressed: RIGHT\n');
        end
        
        this.SetAngularWheelVelocity( ...
            this.TURN_VELOCITY_SLOW, -this.TURN_VELOCITY_SLOW);
        
    case this.CKEY_HEAD_UP % s-key: move head up
        
        this.pitch_angle = this.pitch_angle + 0.01;
        wb_motor_set_position(this.head_pitch, this.pitch_angle);
        
    case this.CKEY_HEAD_DOWN % x-key: move head down
        
        this.pitch_angle = this.pitch_angle - 0.01;
        wb_motor_set_position(this.head_pitch, this.pitch_angle);
        
    case this.CKEY_LIFT_UP % a-key: move lift up
        
        this.lift_angle = this.lift_angle + 0.02;
        this.SetLiftAngle(this.lift_angle);
        
    case this.CKEY_LIFT_DOWN % z-key: move lift down
        
        this.lift_angle = this.lift_angle - 0.02;
        this.SetLiftAngle(this.lift_angle);
        
    case '1' % set lift to pickup position
        
        this.lift_angle = this.LIFT_CENTER;
        this.SetLiftAngle(this.lift_angle);
        
    case '2' % set lift to block +1 position
        
        this.lift_angle = this.LIFT_UP;
        this.SetLiftAngle(this.lift_angle);
        
    case '3' % set lift to highest position
        
        this.lift_angle = this.LIFT_UPUP;
        this.SetLiftAngle(this.lift_angle);
        
    case this.CKEY_UNLOCK % spacebar-key: unlock
        
        if this.locked == true
            
            this.locked = false;
            this.unlockhysteresis = this.HIST;
            wb_connector_unlock(this.con_lift);
            fprintf('UNLOCKED!\n');
        end
        
    otherwise
%         if DEBUG
%             fprintf('Key pressed: NONE\n');
%         end
        this.SetAngularWheelVelocity(0, 0);
        
end % SWITCH(key)

end % FUNCTION ControlCozmo()