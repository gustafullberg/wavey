#ifndef LOAD_CLIENT_HPP
#define LOAD_CLIENT_HPP

class LoadClient {
   public:
    // Returns true if the files could be loaded by an already running instance, false otherwise.
    static bool Load(int argc, char** argv);
};

#endif  // LOAD_CLIENT_HPP
