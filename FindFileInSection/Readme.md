# IDontLikeFileLocks-SectionDumper
dump locked files by duplicating section object handles and mapping them into our process

## How It Works
instead of reading file handles directly, we target **section objects** (mem mapped file representations).

when a process maps a file into memory (common for databases like sqlite), windows creates a section object that backs the mapping. this section object can be mapped into multiple processes simultaneously.

we enumerate the syswide handle table using `NtQuerySystemInformation` to find all handles owned by the target process. for each handle, we:
- duplicate it into our process with `NtDuplicateObject`
- check if it's a Section object type via `NtQueryObject`
- temporarily map it to get the backing filename via `GetMappedFileNameW`
- compare the filename against what we're looking for

once we find the right section, we map it into our address space with `NtMapViewOfSection` (ReadOnly), copy the entire contents to disk, then unmap and close.

## Why This Works 🤔
section objects represent files that are **already mapped into memory**.
mapping a section is not the same as opening a file. the file was opened when the section was created. we're just attaching to existing shared memory.

## Usage
```
IDontLikeFileLocks.exe chrome.exe "Login Data"
```
output goes to `dump.bin` in current directory

## Architecture
tested on x64 Windows 11

## Technical Details
- NT APIs only (ntdll + psapi for path conversion)
- system handle enumeration via `NtQuerySystemInformation(SystemHandleInformation)`
- section identification via `NtQueryObject(ObjectTypeInformation)`
- filename resolution by temporarily mapping + `GetMappedFileNameW` + device name conversion
- final dump via `NtMapViewOfSection` with PAGE_READONLY
- exact filename match only (no substring) to avoid false positives like "Login Data For Account"

## How i done it??
- 1) Step Find handles or dlls using processhacker: <img width="541" height="73" alt="image" src="https://github.com/user-attachments/assets/a327b063-a974-4f5d-bdbb-8962066046ea" />
- 2) Theres bunch of handle types, section,file,mapped we can filter based on that :) : <img width="1054" height="396" alt="image" src="https://github.com/user-attachments/assets/65198b6a-586d-4935-ba52-c4c3ee680ccf" />


## Legal
don't use this on machines you don't own or have permission to test on.  
prison food is mid and they don't let you code 🤔❗

Credits - me
