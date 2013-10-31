#ifndef PKGLIB_IPROGRESS_H
#define PKGLIB_IPROGRESS_H

#include <string>
#include <unistd.h>


namespace APT {
namespace Progress {

 class PackageManager;
 PackageManager* PackageManagerProgressFactory();

 class PackageManager
 {
 private:
    /** \brief dpointer placeholder */
    void *d;

 protected:
    std::string progress_str;
    float percentage;
    int last_reported_progress;

 public:
    PackageManager() 
       : percentage(0.0), last_reported_progress(-1) {};
    virtual ~PackageManager() {};

    /* Global Start/Stop */
    virtual void Start() {};
    virtual void Stop() {};

    /* When dpkg is invoked (may happen multiple times for each 
     * install/remove block 
    */
    virtual void StartDpkg() {};

    virtual pid_t fork() {return fork(); };

    virtual void Pulse() {};
    virtual long GetPulseInterval() {
         return 500000;
    };

    virtual bool StatusChanged(std::string PackageName, 
                               unsigned int StepsDone,
                               unsigned int TotalSteps,
                               std::string HumanReadableAction) ;
    virtual void Error(std::string PackageName,                                
                       unsigned int StepsDone,
                       unsigned int TotalSteps,
                       std::string ErrorMessage) {};
    virtual void ConffilePrompt(std::string PackageName,
                                unsigned int StepsDone,
                                unsigned int TotalSteps,
                                std::string ConfMessage) {};
 };

 class PackageManagerProgressFd : public PackageManager
 {
 protected:
    int OutStatusFd;
    int StepsDone;
    int StepsTotal;
    void WriteToStatusFd(std::string msg);

 public:
    PackageManagerProgressFd(int progress_fd);

    virtual void StartDpkg();
    virtual void Stop();

    virtual bool StatusChanged(std::string PackageName, 
                               unsigned int StepsDone,
                               unsigned int TotalSteps,
                               std::string HumanReadableAction);
    virtual void Error(std::string PackageName,                                
                       unsigned int StepsDone,
                       unsigned int TotalSteps,
                          std::string ErrorMessage);
    virtual void ConffilePrompt(std::string PackageName,
                                unsigned int StepsDone,
                                unsigned int TotalSteps,
                                   std::string ConfMessage);

 };

 class PackageManagerProgressDeb822Fd : public PackageManager
 {
 protected:
    int OutStatusFd;
    int StepsDone;
    int StepsTotal;
    void WriteToStatusFd(std::string msg);

 public:
    PackageManagerProgressDeb822Fd(int progress_fd);

    virtual void Start();
    virtual void Stop();

    virtual bool StatusChanged(std::string PackageName, 
                               unsigned int StepsDone,
                               unsigned int TotalSteps,
                               std::string HumanReadableAction);
    virtual void Error(std::string PackageName,                                
                       unsigned int StepsDone,
                       unsigned int TotalSteps,
                          std::string ErrorMessage);
    virtual void ConffilePrompt(std::string PackageName,
                                unsigned int StepsDone,
                                unsigned int TotalSteps,
                                   std::string ConfMessage);
 };

 class PackageManagerFancy : public PackageManager
 {
 protected:
    int nr_terminal_rows;
    void SetupTerminalScrollArea(int nr_rows);

 public:
    PackageManagerFancy();
    virtual void Start();
    virtual void Stop();
    virtual bool StatusChanged(std::string PackageName, 
                               unsigned int StepsDone,
                               unsigned int TotalSteps,
                               std::string HumanReadableAction);
 };

 class PackageManagerText : public PackageManager
 {
 public:
    virtual bool StatusChanged(std::string PackageName, 
                               unsigned int StepsDone,
                               unsigned int TotalSteps,
                               std::string HumanReadableAction);
 };


}; // namespace Progress
}; // namespace APT

#endif
