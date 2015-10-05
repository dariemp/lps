#include "controller.hxx"
#include "db.hxx"
#include <iostream>
#include <unistd.h>
#include <tbb/tbb.h>

int main(int argc, char *argv[]) {
  try {
    std::cout.setf( std::ios_base::unitbuf );
    int opt;
    std::string dbhost = "127.0.0.1";
    std::string dbname = "icxp";
    std::string dbuser = "class4";
    std::string dbpassword= "class4";
    unsigned int dbport = 5432;
    unsigned int threads_number = 11;
    unsigned int telnet_listen_port = 23;
    unsigned int http_listen_port = 80;
    unsigned int rows_to_read_debug = 0;
    unsigned int refresh_minutes = 30;

    while ((opt = getopt(argc, argv, "c:d:u:p:s:n:t:w:r:m:h")) != -1) {
       switch (opt) {
       case 'c':
          dbhost = std::string(optarg);
          break;
       case 'd':
          dbname = std::string(optarg);
          break;
       case 'u':
          dbuser = std::string(optarg);
          break;
       case 'p':
          dbpassword = std::string(optarg);
          break;
       case 's':
          dbport = atoi(optarg);
          break;
       case 'n':
          threads_number = atoi(optarg);
          break;
       case 't':
          telnet_listen_port = atoi(optarg);
          break;
       case 'w':
          http_listen_port = atoi(optarg);
          break;
       case 'r':
          rows_to_read_debug = atoi(optarg);
          break;
       case 'm':
          refresh_minutes = atoi(optarg);
          break;
       default: /* '?' */
           std::cerr << "Usage: " << argv[0] << " [-h] [-c dbhost] [-d dbname] [-u dbuser] [-p dbpassword]" << std::endl;
           std::cerr << "          [-s dbport] [-t telnet_listen_port] [-w http_listen_port]" << std::endl;
           std::cerr << "          [-n threads_number ] [-r rows_to_read_debug] [-m refresh_minutes]" << std::endl;
           exit(EXIT_FAILURE);
       }
    }
    if (threads_number < 1) {
      std::cerr << "At least one thread is needed to run." << std::endl;
      exit(EXIT_FAILURE);
    }
    std::cout << "Starting..." << std::endl;
    tbb::task_scheduler_init init(threads_number);
    //p_conn_info_t conn_info =  new ConnectionInfo();
    db::ConnectionInfo conn_info;
    conn_info.host = dbhost;
    conn_info.dbname = dbname;
    conn_info.user = dbuser;
    conn_info.password = dbpassword;
    conn_info.port = dbport;
    conn_info.conn_count = threads_number - 1;
    conn_info.rows_to_read_debug = rows_to_read_debug;
    conn_info.refresh_minutes = refresh_minutes;

    ctrl::p_controller_t controller  = ctrl::Controller::get_controller(conn_info, telnet_listen_port, http_listen_port);
    controller->start_workflow();
    return 0;
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}
