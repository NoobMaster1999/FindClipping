#include "Findclipping.h"
#include "PluginComponent.h"
#include "PluginThread.h"
juce_ImplementSingleton (Findclipping)

Findclipping::Findclipping() :
                    numTracks(3),
                    numSources(2),
                    progress(0),
                    needsTraining(false),
                    shouldProcess(false),
                    pluginComponent(nullptr)
{
   
    plugin = new Seperation();
    
    for(int i=0; i < numSources; ++i)
        masks.set(i,new Seperation(10,10));
    


}

Findclipping::~Findclipping()
{    
    if(processThread)
    {
        processThread->signalThreadShouldExit ();
        processThread->stopThread(1000);
    }
    
    clearSingletonInstance();
}


double & Findclipping::getProgress()
{
    return progress;
}


void Findclipping::updateWithNewFiles( const Array<File> & files )
{
    
    const MessageManagerLock mml (Thread::getCurrentThread());
    if (! mml.lockWasGained()) return;
    for(int i=0; i < files.size(); ++i)
        listeners.call (&Findclipping::Listener::engineNewOutputs, files[i], i);
    
    if(!shouldProcess)
        ProgramSettings::processing = false;
    
    
}

void Findclipping::addListener( Findclipping::Listener * listener )
{
    const MessageManagerLock mml (Thread::getCurrentThread());
    if (! mml.lockWasGained()) return;

    listeners.add(listener);

}

void Findclipping::removeListener( Findclipping::Listener * listener)
{
    const MessageManagerLock mml (Thread::getCurrentThread());
    if (! mml.lockWasGained()) return;
    
    listeners.remove(listener);
}


std::map<size_t, size_t> Findclipping::getTraining()
{
    return trainingIndsToSource;
}

void Findclipping::addTraining(unsigned int source, unsigned int xmin, unsigned int xmax)
{
   
    for(size_t i=xmin; i <= xmax; ++i)
        trainingIndsToSource[i] = source+1;
    
    needsTraining = true;
    
    listeners.call (&Findclipping::Listener::engineTrainingChanged);
    postEvent(PluginEvent::Training);
}

void Findclipping::subtractTraining(unsigned int source, unsigned int xmin, unsigned int xmax)
{
    std::map<size_t, size_t>::iterator iter;
    for(size_t i=xmin; i <= xmax; ++i)
    {
        iter = trainingIndsToSource.find(i);
        if(iter!=trainingIndsToSource.end())
            trainingIndsToSource.erase(iter);
    } 
    needsTraining = true;
    
    listeners.call (&Findclipping::Listener::engineTrainingChanged);
    postEvent(PluginEvent::Training);
}
  
void Findclipping::shutdown()
{
    if(processThread)
    {
        processThread->stop();
        processThread->threadShouldExit();
    }
}

void Findclipping::reset()
{
    if(processThread)
	{
        processThread->stop();
		processThread->signalThreadShouldExit();
		processThread->stopThread(1000);
		 
	}
	processThread = nullptr;

    clearTraining();
    
    needsTraining = false;
    
    progress = 0;
}

void Findclipping::clearTraining()
{
    const MessageManagerLock mml (Thread::getCurrentThread());
    if (! mml.lockWasGained()) return;
    
    trainingIndsToSource.clear();
    needsTraining = true;
    listeners.call (&Findclipping::Listener::engineTrainingChanged);
    postEvent(PluginEvent::Training);
}

void Findclipping::start()
{
    shouldProcess = true;
    
    if(processThread)
        processThread->start();
    
    
    ProgramSettings::processing = true;
     
}

void Findclipping::postEvent(audacityPluginEvent event)
{
    if(processThread)
        processThread->postEvent(event);
}

void Findclipping::stop()
{
    
    
    if(processThread)
        processThread->stop();
    
    shouldProcess = false;
    
    if(processThread && !processThread->middleOfProcessing())
        ProgramSettings::processing = false;
    
    
}
