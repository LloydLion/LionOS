#define Throw(text) OS::log(LogLevel::Exception, __FUNCTION__, text); return
#define Warning(text) OS::log(LogLevel::Warning, __FUNCTION__, text)
#define Debug(text) OS::log(LogLevel::Debug, __FUNCTION__, text)

#define ProcessSignature()
#define Process_DisallowManualInvoke() if(!OS::getUnderRunningProcess().isNull) Throw("Manual invoke is disallowed for this process")

#define Delay(time) OS::setDelayTimeout(time); return

#include <ArduinoList.h>
#include <setjmp.h>

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

typedef void (*TaskFunction)(void*);
typedef void (*ProcessTask)();
typedef void (*LoggerFunction)(LogLevel,char*,char*,char*);

struct Task
{
public:
  TaskFunction task;
  unsigned long executionTime;
  void* paramter;
};

struct LongJmpBuffer
{
public:
  bool init;
  jmp_buf buf;
  int val;
};

struct ProcessInfo
{
  bool isNull;
  ProcessTask task;
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
  LongJmpBuffer retPoint;
  int delayTimeout;
  ProcessTask task;
};

int timeTaskSelector(const Task& task);
void* taskProcessSelector(const Process& process);

class Dispatcher
{
public:
  Dispatcher();
  void queryTask(TaskFunction task, int timer, void* arg);
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
  static void tick();
  static int getLeftTime();
  static void initialize();
  static void stopProcess(ProcessTask task);
  static void resumeProcess(ProcessTask task);
  static void log(LogLevel level, char* function, char* text);
  static void setLogger(LoggerFunction logger);
  static void invokeProcess(ProcessTask task);
  static void setMyInterval(int newInterval);
  static ProcessInfo getRunningProcess();
  static ProcessInfo getUnderRunningProcess();
  static ProcessInfo getProcess(ProcessTask task);
  static void setDelayTimeout(int delay);

private:
  static void setReturnPoint();
  static void processInstance(void* data);
  static void processInstanceTimerInvoker(void* data);
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
  unsigned long mil = millis();
  unsigned long et = mil + timer;

  auto taskObj = Task();
  taskObj.task = task;
  taskObj.executionTime = et;
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
    Serial.begin(19200);
  #else
    Serial.begin(LionOS_SerialSpeed);
  #endif
#endif

}

static void OS::invokeProcess(ProcessTask task)
{
  auto process = getProcessDirect(task);

  auto lastUnder = underRunningProcess;
  underRunningProcess = runningProcess;
  runningProcess = process;

  task();

  runningProcess = underRunningProcess;
  underRunningProcess = lastUnder;
}

static void OS::setMyInterval(int newInterval)
{
  runningProcess->interval = newInterval;
}

static void OS::setReturnPoint()
{
  LongJmpBuffer& point = runningProcess->retPoint;
  point.init = true;
  point.val = setjmp(point.buf);
}

static void OS::setDelayTimeout(int delay)
{
  runningProcess->delayTimeout = delay;
  setReturnPoint();
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
      Serial.println("\t\t\tif you see this in release build please contact with developer");
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
  dispatcher->queryTask(processInstance, interval, createProcess(task, interval, name));
}

static void OS::addProcess(ProcessTask task, int interval, char* name, int timer)
{
  dispatcher->queryTask(processInstanceTimerInvoker, timer, createProcess(task, interval, name));
}

static Process* OS::createProcess(ProcessTask task, int interval, char* name)
{
  Process process;
  process.id = processes.getCount();
  process.interval = interval;
  process.task = task;
  process.stopped = false;
  process.name = name;
  process.delayTimeout = -1;
  process.retPoint.init = false;

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
  auto pinfo = (Process*)data;
  runningProcess = pinfo;

  if(pinfo->stopped == false)
  {
    if(pinfo->delayTimeout == -1)
    {
      Serial.println("Direct invoke");
      pinfo->task();
    }
    else
    {
      pinfo->delayTimeout = -1;
      pinfo->retPoint.init = false;
      Serial.println("Longjmp invoke");
      longjmp(pinfo->retPoint.buf, pinfo->retPoint.val);
    }
  }

  if(pinfo->delayTimeout == -1)
  {
    Serial.println("Direct query");
    dispatcher->queryTask(processInstance, pinfo->interval, pinfo);
  }
  else
  {
    Serial.println("Longjmp query");
    dispatcher->queryTask(processInstance, pinfo->delayTimeout, pinfo);
  }

  runningProcess = nullptr;
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
  if(last > 0) delay(last);
  OS::tick();
}

#endif