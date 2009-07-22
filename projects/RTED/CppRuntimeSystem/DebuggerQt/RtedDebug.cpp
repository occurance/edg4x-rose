#include "RtedDebug.h"

#include <QDebug>

#include <pthread.h>


RtedDebug * RtedDebug::single = NULL;

RtedDebug * RtedDebug::instance()
{
    if(!single)
       single = new RtedDebug();

    return single;
}


RtedDebug::RtedDebug()
{
    //here we are in rtsi thread
    rtsi.id = pthread_self();
    rtsi.running=false;
    gui.running=true;

    // creates a new thread for gui, which starts at guiMain
    pthread_create( & gui.id, NULL, &RtedDebug::guiMain, this );
    enterRtsi();
}


// this is the entry-point of gui-thread
void * RtedDebug::guiMain(void * rtedVoid)
{
    RtedDebug * r = static_cast<RtedDebug*>(rtedVoid);

    r->enterGui();
    r->gui.id=pthread_self();


    qDebug() << "In Gui Thread - starting" << pthread_self();

    r->app = new QApplication(0,NULL);
    r->dlg = new DbgMainWindow(r);

    // Initialization finished, go back to rtsi thread
    r->leaveGui();

    r->enterGui();
    r->updateDialogData();
    r->dlg->showMaximized();
    r->app->exec();
    qDebug() << "Exiting because Debugger was closed";
    exit(0);
}


void RtedDebug::on_singleStep()
{
    leaveGui();
    enterGui();
    updateDialogData();
}

void RtedDebug::on_resume()
{
    leaveGui();
    enterGui();
    updateDialogData();
}

void RtedDebug::startGui()
{
    leaveRtsi();
    enterRtsi();
}

void RtedDebug::updateDialogData()
{
    Q_ASSERT(gui.running);

    for(int i=0; i<messages.size(); i++)
    	dlg->addMessage(messages[i].second);

    dlg->updateAllRsData();

    messages.clear();
}




// --------------------- Thread Stuff ------------------------


RtedDebug::ThreadData::ThreadData()
{
    id = -1;
    pthread_mutex_init(& mutex ,NULL);
    pthread_cond_init(& signal,NULL);
    running=false;
}

void RtedDebug::enterThread (ThreadData & cur, ThreadData & other)
{
    pthread_mutex_lock ( & cur.mutex );

    // the signal from other thread signals "finished"
    while(other.running)
        pthread_cond_wait(& cur.signal, & cur.mutex);


    pthread_mutex_lock( & other.mutex);
    cur.running=true;
    other.running=false;
    pthread_mutex_unlock(& other.mutex);
}

void RtedDebug::leaveThread (ThreadData & cur, ThreadData & other )
{
    pthread_mutex_lock(& other.mutex);
    cur.running=false;
    other.running=true;
    pthread_cond_signal(& other.signal);
    pthread_mutex_unlock(& other.mutex);

    pthread_mutex_unlock( & cur.mutex);
}


void RtedDebug::printDbg(const QString & s)
{
    if(gui.id== -1 || rtsi.id== -1)
    {
        qDebug() << "Not yet initialized" << gui.id << rtsi.id ;
        return;
    }

    Q_ASSERT(! (gui.running && rtsi.running) );

    if(gui.running)
    {
        Q_ASSERT(pthread_self() == gui.id);
        qDebug() << "GUI running" << s;
        return;
    }

    if(rtsi.running)
    {
        Q_ASSERT(pthread_self() == rtsi.id);
        qDebug() << "RTSI running" << s;
        return;
    }

    qDebug() << "Nothing running" << s;
}

