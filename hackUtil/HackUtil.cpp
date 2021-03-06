/*  HackUtil is a set of functions that make the use of common "hacking" functions easier.
    Copyright (C) 2010  Adam Heinermann

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "HackUtil.h"

namespace HackUtil
{
  struct memPatch
  {
    void* location;
    char  patch[128];
    u32   patchSize;
  };

  static std::vector<memPatch> changes;
  ///////////////////////////////   LOGGING   //////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////
  void timeLog(const char *filename, const char* format, ...)
  {
    if (filename == NULL || format == NULL)
      return;
    
    char hackbuffer[HU_BUFFER_SIZE];
    va_list ap;
    va_start(ap, format);
    vsnprintf_s(hackbuffer, HU_BUFFER_SIZE, HU_BUFFER_SIZE, format, ap);
    va_end(ap);
    
    FILE* logFile = _fsopen(filename, "a+", _SH_DENYNO);
    if (logFile == NULL)
      return;
      
    timeLog(logFile, "%s", hackbuffer);
    fclose(logFile);
  }
  
  void timeLog(FILE *file, const char* format, ...)
  {
    if (file == NULL || format == NULL)
      return;
      
    char hackbuffer[HU_BUFFER_SIZE];
    va_list ap;
    va_start(ap, format);
    vsnprintf_s(hackbuffer, HU_BUFFER_SIZE, HU_BUFFER_SIZE, format, ap);
    va_end(ap);
    
    SYSTEMTIME time;
    GetLocalTime(&time);
    fprintf_s(file, "[%02d:%02d:%02d:%03d] %s\n", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds, hackbuffer);
    return;
  }
  
  void log(const char *filename, const char* format, ...)
  {
    if (filename == NULL || format == NULL)
      return;
    
    char hackbuffer[HU_BUFFER_SIZE];
    va_list ap;
    va_start(ap, format);
    vsnprintf_s(hackbuffer, HU_BUFFER_SIZE, HU_BUFFER_SIZE, format, ap);
    va_end(ap);
    
    FILE* logFile = _fsopen(filename, "a+", _SH_DENYNO);
    if (logFile == NULL)
      return;
      
    log(logFile, "%s", hackbuffer);
    fclose(logFile);
  }
  
  void log(FILE *file, const char* format, ...)
  {
    if (file == NULL || format == NULL)
      return;
      
    char hackbuffer[HU_BUFFER_SIZE];
    va_list ap;
    va_start(ap, format);
    vsnprintf_s(hackbuffer, HU_BUFFER_SIZE, HU_BUFFER_SIZE, format, ap);
    va_end(ap);
    
    fprintf_s(file, "%s\n", hackbuffer);
    return;
  }
  
  void append(const char *filename, const char* format, ...)
  {
    if (filename == NULL || format == NULL)
      return;
    
    char hackbuffer[HU_BUFFER_SIZE];
    va_list ap;
    va_start(ap, format);
    vsnprintf_s(hackbuffer, HU_BUFFER_SIZE, HU_BUFFER_SIZE, format, ap);
    va_end(ap);
    
    FILE* logFile = _fsopen(filename, "a+", _SH_DENYNO);
    if (logFile == NULL)
      return;
      
    append(logFile, "%s", hackbuffer);
    fclose(logFile);
  }
  
  void append(FILE *file, const char* format, ...)
  {
    if (file == NULL || format == NULL)
      return;
      
    char hackbuffer[HU_BUFFER_SIZE];
    va_list ap;
    va_start(ap, format);
    vsnprintf_s(hackbuffer, HU_BUFFER_SIZE, HU_BUFFER_SIZE, format, ap);
    va_end(ap);
    
    fprintf_s(file, "%s", hackbuffer);
    return;
  }
  
  void logBytes(FILE *file, u8 *data, u32 size)
  {
    if (file == NULL || data == NULL || size == 0)
      return;

    char hackbuffer[HU_BUFFER_SIZE];
    memset(hackbuffer, 0, HU_BUFFER_SIZE);
    for (u32 i = 0; i < size; i++)
    {
      u8 wbyte;
      __try
      {
        wbyte = data[i];
      }
      __except(1)
      {
        wbyte = 0xFF;
      }
      char temp[5];
      sprintf_s(temp, 4, " %02X", wbyte);
      strcat_s(hackbuffer, HU_BUFFER_SIZE, temp);
    }
    fprintf_s(file, "%s", &hackbuffer[1]);
    return;
  }
  
  void logBytes(char *fileName, u8 *data, u32 size)
  {
    if (fileName == NULL || data == NULL || size == 0)
      return;
      
    FILE* logFile = _fsopen(fileName, "a+", _SH_DENYNO);
    if (logFile == NULL)
      return;
      
    logBytes(logFile, data, size);
    fclose(logFile);
  }
  ///////////////////////////////   IMPORTS   //////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////
  IMAGE_IMPORT_DESCRIPTOR* _GetImportDescriptor(HMODULE module)
  {
    IMAGE_DOS_HEADER *mzhead = (IMAGE_DOS_HEADER*)module;
    if (mzhead->e_magic != IMAGE_DOS_SIGNATURE)
      return NULL;

    IMAGE_NT_HEADERS32 *pehead = (IMAGE_NT_HEADERS32*)(mzhead->e_lfanew + (u32)module);
    if (pehead->Signature != IMAGE_NT_SIGNATURE)
      return NULL;
      
    IMAGE_IMPORT_DESCRIPTOR *imports = (IMAGE_IMPORT_DESCRIPTOR*)(pehead->OptionalHeader.DataDirectory[1].VirtualAddress + (u32)module);
    return imports;
  }

  IMAGE_THUNK_DATA32* _GetImportsList(char* sourceModule, char* importModule)
  {
    HMODULE tempModule;
    tempModule = GetModuleHandleA(sourceModule);
    if (tempModule == NULL)
      return NULL;

    IMAGE_IMPORT_DESCRIPTOR *imports = _GetImportDescriptor(tempModule);
    if (imports == NULL)
      return NULL;
    
    for (u32 i = 0; imports[i].Name != 0; i++)
    {
      if (lstrcmpiA((char*)(imports[i].Name + (u32)tempModule), importModule) == 0)
        return (IMAGE_THUNK_DATA32*)(imports[i].OriginalFirstThunk + (u32)tempModule);
    }
    return NULL;
  }

  DWORD* _GetFunctionsList(char* sourceModule, char* importModule)
  {
    HMODULE tempModule = GetModuleHandleA(sourceModule);
    if (tempModule == NULL)
      return NULL;

    IMAGE_IMPORT_DESCRIPTOR *imports = _GetImportDescriptor(tempModule);
    if (imports == NULL)
      return NULL;

    for (u32 i = 0; imports[i].Name != 0; i++)
    {
      if (lstrcmpiA((char*)(imports[i].Name + (u32)tempModule), importModule) == 0)
        return (DWORD*)(imports[i].FirstThunk + (u32)tempModule);
    }
    return NULL;
  }

  bool PatchImport(char* sourceModule, char* importModule, LPCSTR name, void* patchFunction)
  {
    if (name == NULL || patchFunction == NULL || importModule == NULL)
      return false;

    HMODULE tempModule = GetModuleHandleA(sourceModule);
    if (tempModule == NULL)
      return false;

    IMAGE_THUNK_DATA32* importOrigin = _GetImportsList(sourceModule, importModule);
    if (importOrigin == NULL)
      return false;

    DWORD* importFunction = _GetFunctionsList(sourceModule, importModule);
    if (importFunction == NULL)
      return false;

    for (u32 i = 0; importOrigin[i].u1.Ordinal != 0; i++)
    {
      if (IMAGE_SNAP_BY_ORDINAL32(importOrigin[i].u1.Ordinal) && (DWORD)name < 0xFFFF)
      {
        if (IMAGE_ORDINAL32(importOrigin[i].u1.Ordinal) == IMAGE_ORDINAL32((DWORD)name))
        {
          WriteMem(&importFunction[i], &patchFunction, 4);
          return true;
        }
      }
      else
      {
        if (lstrcmpiA(name, (char*)((PIMAGE_IMPORT_BY_NAME)(importOrigin[i].u1.AddressOfData + (DWORD)tempModule))->Name) == 0)
        {
          WriteMem(&importFunction[i], &patchFunction, 4);
          return true;
        }
      }
    }
    return false;
  }

  bool PatchImport(char* importModule, LPCSTR name, void* patchFunction)
  {
    return PatchImport(NULL, importModule, name, patchFunction);
  }

  bool PatchImport(char* sourceModule, char* importModule, int ordinal, void* patchFunction)
  {
    return PatchImport(sourceModule, importModule, (LPCSTR)ordinal, patchFunction);
  }
  
  bool PatchImport(char* importModule, int ordinal, void* patchFunction)
  {
    return PatchImport(NULL, importModule, (LPCSTR)ordinal, patchFunction);
  }

  FARPROC GetImport(char* importModule, LPCSTR name)
  {
    HMODULE module = GetModuleHandleA(importModule);
    if (module == NULL)
      return NULL;

    return GetProcAddress(module, name);
  }
  
  FARPROC GetImport(char *importModule, int ordinal)
  {
    return GetImport(importModule, (LPCSTR)ordinal);
  }

  ///////////////////////////////   MEMORY   //////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////

  void WriteNops(void* dest, u32 size)
  {
    if (dest == NULL || size == 0)
      return;

    memPatch tempPatch;
    tempPatch.location = dest;
    tempPatch.patchSize = size;
    memcpy_s(tempPatch.patch, 128, dest, size);
    changes.push_back(tempPatch);

    DWORD lastAccess = 0;
    VirtualProtect(dest, size, PAGE_EXECUTE_READWRITE, &lastAccess);
    __try
    {
      memset(dest, 0x90, size);
    } 
    __except(1)
    {}
    VirtualProtect(dest, size, lastAccess, &lastAccess);
    return;
  }

  void WriteNops(u32 dest, u32 size)
  {
    WriteNops((void*)dest, size);
    return;
  }

  void WriteMem(void* dest, void* source, u32 size)
  {
    if (dest == NULL || source == NULL || size == 0)
      return;

    memPatch tempPatch;
    tempPatch.location = dest;
    tempPatch.patchSize = size;
    memcpy_s(tempPatch.patch, 128, dest, size);
    changes.push_back(tempPatch);

    WriteMemRaw(dest, source, size);
    return;
  }

  void WriteMemRaw(void* dest, void* source, u32 size)
  {
    if (dest == NULL || source == NULL || size == 0)
      return;

    DWORD lastAccess = 0;
    VirtualProtect(dest, size, PAGE_EXECUTE_READWRITE, &lastAccess);
    __try
    {
      memcpy_s(dest, size, source, size);
    } 
    __except(1)
    {}
    VirtualProtect(dest, size, lastAccess, &lastAccess);
    return;
  }

  void WriteMem(u32 dest, void* source, u32 size)
  {
    WriteMem((void*)dest, source, size);
    return;
  }

  void WriteMemRaw(u32 dest, void* source, u32 size)
  {
    WriteMemRaw((void*)dest, source, size);
    return;
  }

  void JmpPatch(void* dest, void* patch)
  {
    if (dest == NULL || patch == NULL)
      return;
      
    u8 temp[5];
    temp[0] = 0xE9;
    *(int*)&temp[1] = (int)patch - (int)dest - 5;
    WriteMem(dest, temp, 5);
    return;
  }

  void JmpPatch(u32 dest, void* patch)
  {
    JmpPatch((void*)dest, patch);
    return;
  }

  void CallPatch(void* dest, void* patch)
  {
    if (dest == NULL || patch == NULL)
      return;

    u8 temp[5];
    temp[0] = 0xE8;
    *(int*)&temp[1] = (int)patch - (int)dest - 5;
    WriteMem(dest, temp, 5);
    return;
  }

  void CallPatch(u32 dest, void* patch)
  {
    CallPatch((void*)dest, patch);
    return;
  }

  void Revert()
  {
    for each (memPatch i in changes)
      WriteMemRaw(i.location, i.patch, i.patchSize);
    changes.clear();
    return;
  }
  
};
