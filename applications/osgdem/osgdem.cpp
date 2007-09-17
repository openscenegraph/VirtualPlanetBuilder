#include <osg/ArgumentParser>

#include "osgdem.new.cpp"
#include "osgdem.old.cpp"

int main( int argc, char **argv )
{

    // use an ArgumentParser object to manage the program arguments.
    osg::ArgumentParser arguments(&argc,argv);
 
    if (arguments.read("--old"))
    {
        return old_main(arguments);
    }
    else
    {
        return new_main(arguments);
    }
}
