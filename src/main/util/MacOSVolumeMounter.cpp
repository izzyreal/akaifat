#if defined (__APPLE__)

#include "VolumeMounter.h"

#include <sys/stat.h>
#include <unistd.h>

#include <Security/Authorization.h>
#include <Security/AuthorizationTags.h>

#include <vector>

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

int cocoasudo(char *executable, char *commandArgs[], char *icon, char *prompt)
{
    int retVal = 1;
    
    OSStatus status;
    AuthorizationRef authRef;
    
    AuthorizationItem right = { "com.performant.cocoasudo", 0, NULL, 0 };
    AuthorizationRights rightSet = { 1, &right };
    
    AuthorizationEnvironment myAuthorizationEnvironment;
    AuthorizationItem kAuthEnv[2];
    myAuthorizationEnvironment.items = kAuthEnv;
    
    if (prompt && icon)
    {
        kAuthEnv[0].name = kAuthorizationEnvironmentPrompt;
        kAuthEnv[0].valueLength = strlen(prompt);
        kAuthEnv[0].value = prompt;
        kAuthEnv[0].flags = 0;
        
        kAuthEnv[1].name = kAuthorizationEnvironmentIcon;
        kAuthEnv[1].valueLength = strlen(icon);
        kAuthEnv[1].value = icon;
        kAuthEnv[1].flags = 0;
        
        myAuthorizationEnvironment.count = 2;
    }
    else if (prompt)
    {
        kAuthEnv[0].name = kAuthorizationEnvironmentPrompt;
        kAuthEnv[0].valueLength = strlen(prompt);
        kAuthEnv[0].value = prompt;
        kAuthEnv[0].flags = 0;
        
        myAuthorizationEnvironment.count = 1;
    }
    else if (icon)
    {
        kAuthEnv[0].name = kAuthorizationEnvironmentIcon;
        kAuthEnv[0].valueLength = strlen(icon);
        kAuthEnv[0].value = icon;
        kAuthEnv[0].flags = 0;
        
        myAuthorizationEnvironment.count = 1;
    }
    else
    {
        myAuthorizationEnvironment.count = 0;
    }
    
    if (AuthorizationCreate(NULL, &myAuthorizationEnvironment/*kAuthorizationEmptyEnvironment*/, kAuthorizationFlagDefaults, &authRef) != errAuthorizationSuccess)
    {
        printf("Could not create authorization reference object.\n");
        status = errAuthorizationBadAddress;
    }
    else
    {
        status = AuthorizationCopyRights(authRef, &rightSet, &myAuthorizationEnvironment/*kAuthorizationEmptyEnvironment*/,
                                         kAuthorizationFlagDefaults | kAuthorizationFlagPreAuthorize
                                         | kAuthorizationFlagInteractionAllowed | kAuthorizationFlagExtendRights,
                                         NULL);
    }

    if (status == errAuthorizationSuccess)
    {
        FILE *ioPipe;
        char buffer[1024];
        int bytesRead;

        status = AuthorizationExecuteWithPrivileges(authRef, executable, 0, commandArgs, &ioPipe);

        /* Just pipe processes' stdout to our stdout for now; hopefully can add stdin pipe later as well */
        for (;;)
        {
            bytesRead = fread(buffer, sizeof(char), 1024, ioPipe);
            if (bytesRead < 1) break;
            write(STDOUT_FILENO, buffer, bytesRead * sizeof(char));
        }
        
        pid_t pid;
        int pidStatus;
        do {
            pid = wait(&pidStatus);
        } while (pid != -1);
        
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

std::fstream VolumeMounter::mount(std::string bsdName)
{
    const static unsigned int MIN_BSD_NAME_LENGTH = 5;
    
    if (bsdName.length() < MIN_BSD_NAME_LENGTH) {
        printf("bsdName length < 5\n");
        return {};
    }
    
    const bool validStart = bsdName.substr(0, 4) == "disk" || bsdName.substr(0, 5) == "rdisk";
    
    if (!validStart)
    {
        printf("%s is an illegal BSD name. bsdName must begin with disk or rdisk\n", bsdName.c_str());
        return {};
    }
    
    const auto volumePath = "/dev/" + bsdName;
    
    if (!file_exists(volumePath.c_str()))
    {
        printf("Volume path %s does not exist\n", volumePath.c_str());
        return {};
    }
    
    printf("Volume path %s exists\n", volumePath.c_str());
    printf("Attempting Akai FAT16 mount...\n");
    
    std::fstream result;
    
    result.open(volumePath.c_str(), std::ios_base::in);
    
    if (!result.is_open()) {
        char* msg = strerror(errno);
        printf("Failed to open fstream on %s\n", volumePath.c_str());
        printf("Due to: %s\n", msg);
        return {};
    }
    
    return result;
}

#endif
