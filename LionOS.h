#define Throw(text) OS::log(LogLevel::Exception, __FUNCTION__, text); return
#define Warning(text) OS::log(LogLevel::Warning, __FUNCTION__, text)
#define Debug(text) OS::log(LogLevel::Debug, __FUNCTION__, text)

#define ProcessSignature()
#define Process_DisallowManualInvoke() if(!OS::getUnderRunningProcess().isNull) Throw("Manual invoke is disallowed for this process")

#define Delay(time, rpl, rp) OS::returnAfterTo(rp, time); return; rpl:
#define DelayRet(time, rpl, rp, ret) OS::returnAfterTo(rp, time); return ret; rpl:

#define NavTable(rp) switch(rp)
#define NavRecord(rp, rpl) case rp: goto rpl
#define NTE default: Throw("Invalid ReturnPoint index"); case 0: break

#define Var(type, name, val) static type name; name = val

#include <ArduinoList.h>

#ifdef LionOS_IntegrationGyverPower
  #include <GyverPower.h>
  #include <powerConstants.h>
#endif

/*
Task Dispacher for arduino


was made by LloydLion in 2021
*/

enum LogLevel
{
  Exception,
  Warning,
  Debug
};

typedef int ReturnPoint;
typedef void (*TaskFunction)(void*);
typedef void (*ProcessTask)(ReturnPoint);
typedef void (*LoggerFunction)(LogLevel,char*,char*,char*);
typedef ReturnPoint (*StartCondition)();

struct Task
{
public:
  TaskFunction task;
  unsigned long executionTime;
  void* paramter;
};

struct ProcessInfo
{
  bool isNull;
  ProcessTask task;
  StartCondition condition;
  int interval;
  bool stopped;
  char* name;
};

struct Process
{
public:
  int interval;
  int id;
  bool stopped;
  char* name;
  unsigned long baseTime;
  int intervalReplacement;
  ReturnPoint retPoint;
  ProcessTask task;
  StartCondition condition;
};

int timeTaskSelector(const Task& task);
void* taskProcessSelector(const Process& process);

class Dispatcher
{
public:
  Dispatcher();
  void queryTask(TaskFunction task, int timer, void* arg);
  void queryTask(TaskFunction task, void* arg, unsigned long targetTime);
  int getLeftTime();
  void tick();
  List<Task> tasks;
private:
  void recalculateMinTask();

  
  Task minTask;
  int minTaskIndex;
};

class OS
{
public:
  static void queryTask(ProcessTask task, int timer);
  static void addProcess(ProcessTask task, int interval, char* name);
  static void addProcess(ProcessTask task, int interval, char* name, int timer);
  static void addHandlerProcess(ProcessTask task, StartCondition condition, char* name, int checkInterval = 5);
  static void tick();
  static int getLeftTime();
  static void initialize();
  static void stopProcess(ProcessTask task);
  static void resumeProcess(ProcessTask task);
  static void log(LogLevel level, char* function, char* text);
  static void setLogger(LoggerFunction logger);
  static void invokeProcess(ProcessTask task, ReturnPoint rp = 0);
  static void setMyInterval(int newInterval);
  static ProcessInfo getRunningProcess();
  static ProcessInfo getUnderRunningProcess();
  static ProcessInfo getProcess(ProcessTask task);
  static void returnAfterTo(ReturnPoint targetRp, int time);

private:
  static void processInstance(void* data);
  static void processInstanceTimerInvoker(void* data);
  static void handlerProcessInstance(void* data);
  static Process* createProcess(ProcessTask task, int interval, char* name);
  static Process* getProcessDirect(ProcessTask task);
  static ProcessInfo getProcessInfo(Process* process);

  static Dispatcher* dispatcher;
  static List<Process> processes;
  static Process* runningProcess;
  static Process* underRunningProcess;
  static LoggerFunction logger;
};


//---------------------------------------------------------


Dispatcher::Dispatcher()
{
  minTaskIndex = -1;
  minTask.executionTime = -1;
}

void Dispatcher::queryTask(TaskFunction task, int timer, void* arg)
{
  queryTask(task, arg, millis() + (unsigned long)timer);
}

void Dispatcher::queryTask(TaskFunction task, void* arg, unsigned long targetTime)
{
  auto taskObj = Task();
  taskObj.task = task;
  taskObj.executionTime = targetTime;
  taskObj.paramter = arg;
  tasks.add(taskObj);
  recalculateMinTask();
}

void Dispatcher::tick()
{
  if(minTaskIndex != -1 && millis() >= minTask.executionTime)
  {
#ifdef LionOS_OSDebug
    Serial.println("-----------------------");
    Serial.print("millis: ");
    Serial.println(millis());
    Serial.println("-----------------------");
    auto node = tasks.getNode(0);
    do
    {
      Serial.print("Task: [");
      Serial.print("executionTime:");
      Serial.print(node->object.executionTime);
      Serial.print("| task:");
      Serial.print((int)node->object.task);
      Serial.print("| parameter:");
      Serial.print((int)node->object.paramter);
      Serial.println("]");
    }
    while((node = node->next) != nullptr);
    Serial.println("-----------------------");
    Serial.println("");
#endif

    minTask.task(minTask.paramter);
    tasks.remove(minTaskIndex);
    recalculateMinTask();    
  }
}

int Dispatcher::getLeftTime()
{
  return minTask.executionTime - millis();
}

void Dispatcher::recalculateMinTask()
{
  minTaskIndex = tasks.minVal(timeTaskSelector);
  minTask = tasks.get(minTaskIndex);
}

static Dispatcher* OS::dispatcher;
static List<Process> OS::processes;
static Process* OS::runningProcess;
static Process* OS::underRunningProcess;
static LoggerFunction OS::logger;

static void OS::initialize()
{
  dispatcher = new Dispatcher();
  runningProcess = nullptr;
  underRunningProcess = nullptr;
  logger = nullptr;


#ifndef LionOS_NoAutoSerial
  #ifndef LionOS_SerialSpeed
    Serial.begin(38400);
  #else
    Serial.begin(LionOS_SerialSpeed);
  #endif
#endif

#ifdef LionOS_IntegrationGyverPower
  #ifdef LionOS_IGP_ManualWDCalibrate
    power.calibrate(LionOS_IGP_ManualWDCalibrate);
  #else
    power.autoCalibrate();
  #endif
#endif
}

static void OS::returnAfterTo(ReturnPoint targetRp, int time)
{
  if(underRunningProcess == nullptr)
  {
    runningProcess->retPoint = targetRp;
    runningProcess->intervalReplacement = time;
  }
  else
  {
    underRunningProcess->retPoint = targetRp;
    underRunningProcess->intervalReplacement = time;
  }
}

static void OS::invokeProcess(ProcessTask task, ReturnPoint rp = 0)
{
  auto process = getProcessDirect(task);

  auto lastUnder = underRunningProcess;
  underRunningProcess = runningProcess;
  runningProcess = process;

  task(rp);

  runningProcess = underRunningProcess;
  underRunningProcess = lastUnder;
}

static void OS::setMyInterval(int newInterval)
{
  runningProcess->interval = newInterval;
}

static ProcessInfo OS::getRunningProcess()
{
  return getProcessInfo(runningProcess);
}

static ProcessInfo OS::getUnderRunningProcess()
{
  return getProcessInfo(underRunningProcess);
}

static ProcessInfo OS::getProcess(ProcessTask task)
{
  return getProcessInfo(getProcessDirect(task));
}

static void OS::setLogger(LoggerFunction logger)
{
  OS::logger = logger;
}

static void OS::stopProcess(ProcessTask task)
{
  auto process = getProcessDirect(task);
  process->stopped = true;
}

static void OS::resumeProcess(ProcessTask task)
{
  auto process = getProcessDirect(task);
  process->stopped = false;
}

static Process* OS::getProcessDirect(ProcessTask task)
{
  return &processes.get(processes.find(taskProcessSelector, (void*)task));
}

static ProcessInfo OS::getProcessInfo(Process* process)
{
  ProcessInfo info;
  if(process == nullptr) 
  {
    info.isNull = true;
  }
  else
  {
    info.isNull = false;
    info.name = process->name;
    info.interval = process->interval;
    info.stopped = process->stopped;
    info.task = process->task;
    info.condition = process->condition;
  }

  return info;
}

static int OS::getLeftTime()
{
  return dispatcher->getLeftTime();
}

static void OS::log(LogLevel level, char* function, char* text)
{
  if(logger != nullptr) logger(level, runningProcess->name, function, text);

  switch(level)
  {
    case LogLevel::Debug:
#ifndef LionOS_NoProgramDebug
      if(underRunningProcess != nullptr) 
      {
        Serial.print(underRunningProcess->name);
        Serial.print("->");
      }
      Serial.print(runningProcess->name);
      Serial.print("|");
      Serial.print(function);
      Serial.print(" [DEBUG]: ");
      Serial.println(text);
#endif
    break;

    case LogLevel::Exception:
      Serial.println("!!!!!!! [Exception in program work] !!!!!!!");
      Serial.println("\t\tif you see this in release build please contact with developer");
      Serial.print("\tProcess: ");
      if(underRunningProcess != nullptr) 
      {
        Serial.print(underRunningProcess->name);
        Serial.print("->");
      }
      Serial.println(runningProcess->name);
      Serial.print("\tFunction: ");
      Serial.println(function);
      Serial.print("\tDebug message: ");
      Serial.println(text);
      Serial.println();
    break;

    case LogLevel::Warning:
      if(underRunningProcess != nullptr) 
      {
        Serial.print(underRunningProcess->name);
        Serial.print("->");
      }
      Serial.print(runningProcess->name);
      Serial.print("|");
      Serial.print(function);
      Serial.print(" [Warning]: ");
      Serial.println(text);
    break;
  }
}

static void OS::queryTask(ProcessTask task, int timer)
{
  dispatcher->queryTask(task, timer, nullptr);
}

static void OS::addProcess(ProcessTask task, int interval, char* name)
{
  dispatcher->queryTask(processInstance, createProcess(task, interval, name), interval);
}

static void OS::addProcess(ProcessTask task, int interval, char* name, int timer)
{
  auto process = createProcess(task, interval, name);
  process->baseTime += timer;
  dispatcher->queryTask(processInstanceTimerInvoker, process, timer);
}

static void OS::addHandlerProcess(ProcessTask task, StartCondition condition, char* name, int checkInterval = 5)
{
  auto process = createProcess(task, checkInterval, name);
  process->condition = condition;
  dispatcher->queryTask(handlerProcessInstance, process, checkInterval);
}

static Process* OS::createProcess(ProcessTask task, int interval, char* name)
{
  Process process;
  process.id = processes.getCount();
  process.interval = interval;
  process.task = task;
  process.stopped = false;
  process.name = name;
  process.baseTime = interval;
  process.retPoint = 0;
  process.intervalReplacement = -1;
  process.condition = nullptr;

  processes.add(process);

  // Can't return $process. List copy element!
  return &processes.get(processes.getCount() - 1);
}

static void OS::tick()
{
  dispatcher->tick();
}

static void OS::processInstance(void* data)
{
  auto process = (Process*)data;
  runningProcess = process;

  auto rp = process->retPoint;
  process->retPoint = 0;

  if(process->stopped == false) process->task(rp);

  process->baseTime += process->intervalReplacement == -1 ? process->interval : process->intervalReplacement;

  process->intervalReplacement = -1;
  runningProcess = nullptr;

  if(process->condition == nullptr) dispatcher->queryTask(processInstance, process, process->baseTime);
}

static void OS::handlerProcessInstance(void* data)
{
  auto process = (Process*)data;
  auto rp = process->condition();
  bool lastStopped = process->stopped;

  if(rp != -1)
  {
    process->retPoint = rp;
  }
  else
  {
    process->stoppped = true;
  }

  processInstance(data);

  if()



  dispatcher->queryTask(handlerProcessInstance, process, process->baseTime);
}

static void OS::processInstanceTimerInvoker(void* data)
{
  dispatcher->queryTask(processInstance, ((Process*)data)->interval, data);
}

int timeTaskSelector(const Task& task)
{
  return task.executionTime - millis();
}

void* taskProcessSelector(const Process& process)
{
  return (void*)process.task;
}

#ifdef LionOS_FrameMode

void initialize();

void setup()
{
  OS::initialize();
  initialize();
}

void loop()
{
  auto last = OS::getLeftTime();


#ifndef LionOS_IntegrationGyverPower
  if(last > 0) delay(last);
#else
  int serial = Serial.availableForWrite() / 5;
  delay(serial); //Serial send
  if(last > 16 + serial) delay(power.sleepDelay(last - serial));
#endif

  OS::tick();
}

#endif



