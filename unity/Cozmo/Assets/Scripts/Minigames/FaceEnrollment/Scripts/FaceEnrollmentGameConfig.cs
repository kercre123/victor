namespace FaceEnrollment {
  public class FaceEnrollmentGameConfig : ChallengeConfigBase {

    public long UpdateThresholdLastSeenSeconds;
    public long UpdateThresholdLastEnrolledSeconds;

    public override int NumCubesRequired() {
      return 0;
    }

    public override int NumPlayersRequired() {
      return 1;
    }
  }
}