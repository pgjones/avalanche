#ifndef __AVALANCHE_H__
#define __AVALANCHE_H__

#include <string>
#include <vector>
#include <map>
#include <queue>
#include <pthread.h>

class TObject;

#ifdef __MAKECINT__
#define pthread_t void
#define pthread_mutex_t void
#endif

namespace Json {
    class Value;
}

/**
 * Namespace containing all of the avalanche library
 */
namespace avalanche {

    /**
     * Functor that turns a document from CouchDB into a TObject. An instance
     * must be specified for a new database connection.
     * @see client::addDB()
     */
    class docObjectMap {
        public:
            /** Convert JSON data to a TObject* */
            virtual TObject* operator()(const Json::Value& v) = 0;
    };

    /**
     * Container holding the state of a data stream
     * Used to pass state information to a stream's "watcher" thread
     */
    class streamState {
        public:
            std::queue<TObject*>* queue; //!< Queue where incoming data is held
            pthread_mutex_t* queueMutex; //!< Lock protecting the queue
    };

    /**
     * An avalanche data stream client
     *
     * The client receives TObjects from both dispatcher streams (via
     * libzdispatch) and from CouchDB databases, aggregating events from
     * the builder and headers in the database into one homogeneous stream.
     *
     * Clients may connect to unlimited numbers of dispatchers and databases,
     * but data from different sources is not guaranteed to be ordered.
     */
    class client {
        public:
            /** Create a new client */
            client();

            /**
             * Destroy this client, closing any open connections and
             * terminating any running threads
             */
            ~client();

            /**
             * Connect to a dispatcher stream. This may be called repeatedly
             * to "watch" many streams with data interleaved.
             * @param _addr The address of the dispatcher server
             * @param _records Optional dispatcher subscription string
             */
            void addDispatcher(std::string _addr, std::string _records="w RECHDR w RAWDATA");

            /**
             * Connect to a CouchDB database, watching the changes feed for
             * header data. This may be called repeatedly to "watch" many
             * databases.
             * @param _host Hostname of the CouchDB server, optionally
             *              including login information like:
             *              http://user:password@host:port
             * @param _dbname Name of the database to watch
             * @param _map A functor converting JSON to TObjects; see @ref docObjectMap
             * @param _filterName Name of the CouchDB filter function to apply to
             *                    the changes feed
             */
            void addDB(std::string _host, std::string _dbname, docObjectMap& _map, std::string _filterName="");

            /**
             * Get the lists of connected dispatchers and databases
             * @return A map (keys "dispatcher", "couchdb") of lists of
             *         connection identifier strings
             */
            const std::map<std::string, std::vector<std::string> > getStreamList() { return streams; }

            /**
             * Receive the next available record
             * @param blocking If true, wait until data is available to return
             * @return The next available TObject
             */
            TObject* recv(bool blocking=true);

        protected:
            std::queue<TObject*> queue; //!< Buffer of received objects
            pthread_mutex_t* queueMutex; //!< Mutex protection for queue

            /** A map serving as a list of "watcher" threads and their state */
            std::map<pthread_t*, streamState*> threads;

            /**
             * A map with lists of connected streams
             * e.g. streams["dispatcher"] -> ["tcp://localhost:5024", ...]
             */
            std::map<std::string, std::vector<std::string> > streams;
    };

} // namespace avalanche

#endif

