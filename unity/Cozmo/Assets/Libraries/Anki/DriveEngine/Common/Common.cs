using System;
using UnityEngine;
using System.Runtime.InteropServices;

namespace Anki {

  [AttributeUsage(AttributeTargets.Method)]
  public class MonoPInvokeCallbackAttribute : System.Attribute {
    public MonoPInvokeCallbackAttribute(Type T) {
    }
  }

  static class EnumFlags {
    public static bool HasFlag(this Enum e, Enum flag) {
      return ((Convert.ToUInt64(e) & Convert.ToUInt64(flag)) == Convert.ToUInt64(flag));
    }
  }

  public enum OperationStatus : byte {
    Success,
    Failure
  }

  public enum UpdateType : byte {
    Appeared,
    Disappeared
  }

  public enum Proximity : byte {
    Unknown,
    Close
  }

  public enum ConnectionState : byte {
    Unavailable,
    Disconnected,
    Connecting,
    Connected
  }

  public enum DesiredConnectionState : byte {
    Disconnected = ConnectionState.Disconnected,
    Connected = ConnectionState.Connected,
  }

  [Flags]
  public enum BatteryState : byte {
    Normal = 0,
    Low = 1 << 0,
    Full = 1 << 1,
    OnCharger = 1 << 2,
  }

  public enum CategoryUpgradeTransactionType : byte {
    Purchase,
    Sale
  }

  [Flags]
  public enum FirmwareUpdateOptions : uint {
    None = 0,
    NoDisconnect = (1 << 0),
    Bootloader = (1 << 1),
    Force = (1 << 2),
  }

  public enum FirmwareUpdateStatus : byte {
    Success,
    InProgress,
    ErrorTimeout,
    ErrorDisconnect,
    ErrorBootloaderVersion,
    ErrorMigration
  }

  [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
  public struct AdvertisementData {
    public byte CacheId;
    public ConnectionState ConnectionState;
    public BatteryState BatteryState;
    public Proximity Proximity;
    public Int32 RSSI;
    public UInt16 FirmwareVersion;
    public UInt16 Bounty;
    public UInt32 UpgradesAndItems;
    [MarshalAs(UnmanagedType.ByValTStr, ArraySubType = UnmanagedType.U1, SizeConst = 13)]
    public string
      UTF8Name;
  }

  public enum ParentMergeType {
    One,
    All
  }
	
}