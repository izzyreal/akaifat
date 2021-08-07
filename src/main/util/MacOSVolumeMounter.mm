#if defined (__APPLE__)

#include "VolumeMounter.h"

#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>

#include <Foundation/Foundation.h>

#include <Security/Authorization.h>
#include <Security/AuthorizationTags.h>
#include <SystemConfiguration/SystemConfiguration.h>

#include <string>

using namespace akaifat::util;

// copied from cocoasudo https://github.com/performantdesign/cocoasudo/

char *addfiletopath(const char *path, const char *filename)
{
    char *outbuf;
    char *lc;
    
    lc = (char *)path + strlen(path) - 1;
    if (lc < path || *lc != '/')
    {
        lc = NULL;
    }
    while (*filename == '/')
    {
        filename++;
    }
    outbuf = (char*) malloc(strlen(path) + strlen(filename) + 1 + (lc == NULL ? 1 : 0));
    sprintf(outbuf, "%s%s%s", path, (lc == NULL) ? "/" : "", filename);
    
    return outbuf;
}

int isexecfile(const char *name)
{
    struct stat s;
    return (!access(name, X_OK) && !stat(name, &s) && S_ISREG(s.st_mode));
}

char *which(const char *filename)
{
    char *path, *p, *n;
    
    path = getenv("PATH");
    if (!path)
    {
        return NULL;
    }
    
    p = path = strdup(path);
    while (p) {
        n = strchr(p, ':');
        if (n)
        {
            *n++ = '\0';
        }
        if (*p != '\0')
        {
            p = addfiletopath(p, filename);
            if (isexecfile(p))
            {
                free(path);
                return p;
            }
            free(p);
        }
        p = n;
    }
    free(path);
    return NULL;
}

int cocoasudo(char *executable, char *commandArgs[], char *prompt)
{
    int retVal = 1;
    
    OSStatus status;
    AuthorizationRef authRef;
    
    AuthorizationItem right = { "com.performant.cocoasudo", 0, NULL, 0 };
    AuthorizationRights rightSet = { 1, &right };
    
    AuthorizationEnvironment myAuthorizationEnvironment;
    AuthorizationItem kAuthEnv[2];
    myAuthorizationEnvironment.items = kAuthEnv;
    
    kAuthEnv[0].name = kAuthorizationEnvironmentPrompt;
    kAuthEnv[0].valueLength = strlen(prompt);
    kAuthEnv[0].value = prompt;
    kAuthEnv[0].flags = 0;
    
    myAuthorizationEnvironment.count = 1;
    
    if (AuthorizationCreate(NULL, &myAuthorizationEnvironment, kAuthorizationFlagDefaults, &authRef) != errAuthorizationSuccess)
    {
        printf("Could not create authorization reference object.\n");
        status = errAuthorizationBadAddress;
    }
    else
    {
        status = AuthorizationCopyRights(authRef, &rightSet, &myAuthorizationEnvironment,
                                         kAuthorizationFlagDefaults | kAuthorizationFlagPreAuthorize
                                         | kAuthorizationFlagInteractionAllowed | kAuthorizationFlagExtendRights,
                                         NULL);
    }
    
    if (status == errAuthorizationSuccess)
    {
        FILE *ioPipe = NULL;
        char buffer[1024];
        int bytesRead;
        
        status = AuthorizationExecuteWithPrivileges(authRef, executable, 0, commandArgs, &ioPipe);

        pid_t pid;
        int pidStatus;
        pid = wait(&pidStatus);
        
        if (ioPipe != NULL)
            close(fileno(ioPipe));
        
        if (status == errAuthorizationSuccess)
        {
            retVal = 0;
        }
    }
    else
    {
        AuthorizationFree(authRef, kAuthorizationFlagDestroyRights);
        authRef = NULL;
        if (status != errAuthorizationCanceled)
        {
            // pre-auth failed
            printf("Pre-auth failed\n");
        }
    }
    
    return retVal;
}

// end of cocoasudo


inline bool file_exists (const std::string& name) {
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}

void unmountFromMacOS(std::string volumePath)
{
    const std::string cmd = "diskutil unmount " + volumePath;
    system(cmd.c_str());
}

void mountToMacOS(std::string volumePath)
{
    const std::string cmd = "diskutil mount " + volumePath;
    system(cmd.c_str());
}

std::string getCurrentDesktopUser()
{
    SCDynamicStoreRef store;
    CFStringRef name;
    uid_t uid;
    size_t BUFLEN = 256;
    char buf[BUFLEN];
    Boolean ok;

    store = SCDynamicStoreCreate(NULL, CFSTR("GetConsoleUser"), NULL, NULL);
    assert(store != NULL);
    name = SCDynamicStoreCopyConsoleUser(store, &uid, NULL);
    CFRelease(store);

    if (name != NULL) {
        ok = CFStringGetCString(name, buf, BUFLEN, kCFStringEncodingUTF8);
        assert(ok == true);
        CFRelease(name);
    } else {
        strcpy(buf, "<none>");
    }

    printf("%d %s\n", uid, buf);
    return std::string(buf);
}

void demotePermissions(std::string volumePath)
{
    struct stat info;
    stat(volumePath.c_str(), &info);
    struct passwd *pw = getpwuid(info.st_uid);
    char* currentOwnerUserName = pw->pw_name;
    std::string currentUser = getCurrentDesktopUser();
    
    if (currentOwnerUserName == currentUser) {
        std::string cmdStr = "/bin/chmod 626 " + volumePath;
        system(cmdStr.c_str());
        return;
    }
        
    std::string cmdStr = "/usr/sbin/chown";
    char* cmd = const_cast<char*>(cmdStr.c_str());
    
    std::string titleStr = "Please approve temporary raw read/write permissions for " + volumePath + ".\nThe owner of this path will also temporarily change to the current user instead of root.\n\nAfter reinserting the medium the owner will be root again.\nThe permissions will be automatically reset to read-only when the application shuts down.";
    
    char* title = const_cast<char*>(titleStr.c_str());

    char* argv[0];
    std::string arg1Str = currentUser;
    std::string arg2Str = volumePath;
    
    char* commandArgs[3];
    
    commandArgs[0] = const_cast<char*>(arg1Str.c_str());
    commandArgs[1] = const_cast<char*>(arg2Str.c_str());
    commandArgs[2] = nil;
    
    cocoasudo(cmd, commandArgs, title);
    
    std::string cmdStr2 = "/bin/chmod 626 " + volumePath;
    system(cmdStr2.c_str());
}

void repairPermissions(std::string volumePath)
{
    std::string cmdStr = "/bin/chmod 640 " + volumePath;
    system(cmdStr.c_str());
}

bool validateBSDName(std::string bsdName)
{
    const static unsigned int MIN_BSD_NAME_LENGTH = 5;
    bool valid = true;
    
    if (bsdName.length() < MIN_BSD_NAME_LENGTH) {
        printf("bsdName length < 5\n");
        valid = false;
    }
    
    const bool validStart = bsdName.substr(0, 4) == "disk" || bsdName.substr(0, 5) == "rdisk";
    
    if (!validStart)
    {
        printf("%s is an illegal BSD name. bsdName must begin with disk or rdisk\n", bsdName.c_str());
        valid = false;
    }
    
    const auto volumePath = "/dev/" + bsdName;
    
    if (!file_exists(volumePath.c_str()))
    {
        printf("Volume path %s does not exist\n", volumePath.c_str());
        valid = false;
    }
    
    return valid;
}

std::fstream VolumeMounter::mount(std::string bsdName, bool readOnly)
{
    if (!validateBSDName(bsdName))
        return {};
    
    const auto volumePath = "/dev/" + bsdName;

    unmountFromMacOS(volumePath);
    demotePermissions(volumePath);
    
    printf("Volume path %s exists\n", volumePath.c_str());
    printf("Attempting Akai FAT16 mount...\n");
    
    std::fstream result;
    
    result.open(volumePath.c_str(), (readOnly ? (std::ios_base::in) : (std::ios_base::in | std::ios_base::out)));
    
    if (!result.is_open()) {
        char* msg = strerror(errno);
        printf("Failed to open fstream on %s\n", volumePath.c_str());
        printf("Due to: %s\n", msg);
        return {};
    }
    
    return result;
}

void VolumeMounter::unmount(std::string bsdName)
{
    if (!validateBSDName(bsdName))
        return;
    
    const auto volumePath = "/dev/" + bsdName;
    repairPermissions(volumePath);
    mountToMacOS(volumePath);
}

#endif
