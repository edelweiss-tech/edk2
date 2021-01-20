/**@file

Module Name:

  BaikalRtc.c

Abstract:

  RTC for baikal (STAB!!!)

**/

#include <Uefi.h>
#include <Protocol/RealTimeClock.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include "i2c_dw.h"

EFI_TIME defaultTime = {
  2000, // Year;
  1,    // Month;
  1,    // Day;
  0,    // Hour;
  0,    // Minute;
  0,    // Second;
  0,    // Pad1;
  0,    // Nanosecond;
  0,    // TimeZone;
  0,    // Daylight;
  0,    // Pad2;
};

BOOLEAN
DayValid (
  IN  EFI_TIME  *Time
  );

BOOLEAN
IsLeapYear (
  IN EFI_TIME   *Time
  );

EFI_STATUS
RtcTimeFieldsValid (
  IN EFI_TIME *Time
  );

EFI_STATUS
EFIAPI
InitializeRealTimeClock (
  IN EFI_HANDLE                          ImageHandle,
  IN EFI_SYSTEM_TABLE                    *SystemTable
  );


EFI_STATUS
EFIAPI
BaikalGetTime (
  OUT EFI_TIME                                 *Time,
  OUT EFI_TIME_CAPABILITIES                    *Capabilities OPTIONAL
  )
/*++

Routine Description:
  Service routine for RealTimeClockInstance->GetTime

Arguments:

  Time          - A pointer to storage that will receive a snapshot of the current time.

  Capabilities  - A pointer to storage that will receive the capabilities of the real time clock
                  in the platform. This includes the real time clock's resolution and accuracy.
                  All reported device capabilities are rounded up.  This is an OPTIONAL argument.

Returns:

  EFI_SUCEESS   - The underlying GetSystemTime call occurred and returned
                  Note that in the NT32 emulation, the GetSystemTime call has no return value
                  thus you will always receive a EFI_SUCCESS on this.

--*/
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
{
  EFI_STATUS            Status;

  if (EfiAtRuntime()) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check parameter for null pointer
  //
  if (Time == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = get_time(Time);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = RtcTimeFieldsValid (Time);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Capabilities != NULL) {
    Capabilities->Resolution  = 1;
    Capabilities->Accuracy    = 50000000;
    Capabilities->SetsToZero  = FALSE;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BaikalSetTime (
  IN EFI_TIME   *Time
  )
/*++

Routine Description:
  Service routine for RealTimeClockInstance->SetTime

Arguments:

  Time          - A pointer to storage containing the time and date information to
                  program into the real time clock.

Returns:

  EFI_SUCEESS           - The operation completed successfully.

  EFI_INVALID_PARAMETER - One of the fields in Time is out of range.

  EFI_DEVICE_ERROR      - The operation could not be complete due to a device error.

--*/
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS            Status;

  if (EfiAtRuntime()) {
    return EFI_UNSUPPORTED;
  }

  if (Time == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Make sure that the time fields are valid
  //
  Status = RtcTimeFieldsValid (Time);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = set_time(Time);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BaikalGetWakeupTime (
  OUT BOOLEAN        *Enabled,
  OUT BOOLEAN        *Pending,
  OUT EFI_TIME       *Time
  )
/*++

Routine Description:
  Service routine for RealTimeClockInstance->GetWakeupTime

Arguments:
  This          - Indicates the protocol instance structure.

  Enabled       - Indicates if the alarm is currently enabled or disabled.

  Pending       - Indicates if the alarm signal is pending and requires
                  acknowledgement.

  Time          - The current alarm setting.

Returns:

  EFI_SUCEESS           - The operation completed successfully.

  EFI_DEVICE_ERROR      - The operation could not be complete due to a device error.

  EFI_UNSUPPORTED       - The operation is not supported on this platform.

--*/
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
BaikalSetWakeupTime (
  IN BOOLEAN      Enable,
  OUT EFI_TIME    *Time
  )
/*++

Routine Description:
  Service routine for RealTimeClockInstance->SetWakeupTime

Arguments:

  Enabled       - Enable or disable the wakeup alarm.

  Time          - If enable is TRUE, the time to set the wakup alarm for.
                  If enable is FALSE, then this parameter is optional, and
                  may be NULL.

Returns:

  EFI_SUCEESS           - The operation completed successfully.

  EFI_DEVICE_ERROR      - The operation could not be complete due to a device error.

  EFI_INVALID_PARAMETER - A field in Time is out of range.

  EFI_UNSUPPORTED       - The operation is not supported on this platform.

--*/
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
InitializeRealTimeClock (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
/*++

Routine Description:
  Install Real Time Clock Protocol

Arguments:
  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns:

  EFI_SUCEESS - Real Time Clock Services are installed into the Runtime Services Table

--*/
// TODO:    ImageHandle - add argument and description to function comment
// TODO:    SystemTable - add argument and description to function comment
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  /* CHECK */
  EFI_TIME  Time;
  if (get_time(&Time)){
    return EFI_DEVICE_ERROR;
  }
  if (RtcTimeFieldsValid(&Time)){
    set_time(&defaultTime);
  }

  SystemTable->RuntimeServices->GetTime       = BaikalGetTime;
  SystemTable->RuntimeServices->SetTime       = BaikalSetTime;
  SystemTable->RuntimeServices->GetWakeupTime = BaikalGetWakeupTime;
  SystemTable->RuntimeServices->SetWakeupTime = BaikalSetWakeupTime;

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiRealTimeClockArchProtocolGuid,
                  NULL,
                  NULL
                  );
  return Status;
}

EFI_STATUS
RtcTimeFieldsValid (
  IN EFI_TIME *Time
  )
/*++

Routine Description:

  Arguments:

  Returns:
--*/
// TODO:    Time - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  if (Time->Year < 1998 ||
      Time->Year > 2099 ||
      Time->Month < 1 ||
      Time->Month > 12 ||
      (!DayValid (Time)) ||
      Time->Hour > 23 ||
      Time->Minute > 59 ||
      Time->Second > 59 ||
      Time->Nanosecond > 999999999 ||
      (!(Time->TimeZone == EFI_UNSPECIFIED_TIMEZONE || (Time->TimeZone >= -1440 && Time->TimeZone <= 1440))) ||
      (Time->Daylight & (~(EFI_TIME_ADJUST_DAYLIGHT | EFI_TIME_IN_DAYLIGHT)))
      ) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

BOOLEAN
DayValid (
  IN  EFI_TIME  *Time
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Time  - TODO: add argument description

Returns:

  TODO: add return values

--*/
{

  INTN  DayOfMonth[12] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  if (Time->Day < 1 ||
      Time->Day > DayOfMonth[Time->Month - 1] ||
      (Time->Month == 2 && (!IsLeapYear (Time) && Time->Day > 28))
      ) {
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
IsLeapYear (
  IN EFI_TIME   *Time
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Time  - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  if (Time->Year % 4 == 0) {
    if (Time->Year % 100 == 0) {
      if (Time->Year % 400 == 0) {
        return TRUE;
      } else {
        return FALSE;
      }
    } else {
      return TRUE;
    }
  } else {
    return FALSE;
  }
}
