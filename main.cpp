#include <iostream>
//#include "fwd.h"
//#include "dbg.h"
#include "asio.hpp"

using namespace std;
using namespace asio;

/**consiste de un socket cliente, un puerto serial, un nombre de servicio a proveer*/
class fwd
{
public:
  fwd(asio::io_service& io_service, std::string ip, std::string puerto_tcp, std::string nombre_puerto_COM, unsigned int baudios) :
    iosvc_(io_service),

    nombre_puerto_com_(nombre_puerto_COM),
    baudios_(baudios),
    puerto_com_(iosvc_, nombre_puerto_com_),

    socket_(io_service)
  {
    asio::ip::tcp::resolver resolvedor(io_service);
    asio::ip::tcp::resolver::query query(ip, puerto_tcp); //puedes meter dns aqui
    asio::ip::tcp::resolver::iterator iter = resolvedor.resolve(query);
    endpoint_ = iter->endpoint();

    puerto_com_.set_option(asio::serial_port_base::baud_rate(baudios_));
  }
  void iniciar()
  {
    leer_puerto_serial();
    conectar_socket();
  }

  void leer_puerto_serial();
  void escribir_puerto_serial(std::string str);

  void conectar_socket();
  void leer_socket();
  void escribir_socket(std::string str);


private:
  asio::io_service& iosvc_;

  std::string nombre_puerto_com_;
  unsigned int baudios_;
  asio::serial_port puerto_com_;
  std::array<char,4096> rx_buf_pser_;
  std::string tx_buf_pser_;

  std::array<char,4096> rx_buf_socket_;
  std::string tx_buf_socket_;
  asio::ip::tcp::socket socket_;
  asio::ip::tcp::endpoint endpoint_;

};


void fwd::leer_puerto_serial()
{
  puerto_com_.async_read_some(asio::buffer(rx_buf_pser_),
    [this](std::error_code ec, std::size_t length)
    {
      if(!ec)
      {
        cout.write(rx_buf_pser_.data(), length);

        /**retransmitir a la nube*/
        escribir_socket(string(rx_buf_pser_.data()));

        memset(rx_buf_pser_.data(), '\0', rx_buf_pser_.size() );
        leer_puerto_serial();
      }

      else
        cout << ec.value() << " " << ec.message() << endl;
    });
}

void fwd::escribir_puerto_serial(string s) //es asíncrono pero no hacemos nada al terminar
{
  tx_buf_pser_ = s;
  asio::async_write(puerto_com_, asio::buffer(tx_buf_pser_.c_str(), tx_buf_pser_.size()),
    [this] (std::error_code ec, size_t len)
  {
    if(ec)
      cout << ec.value() << ec.message() << endl;
  });
}

/**              puerto_serie                    */
/**                 socket                       */

void fwd::conectar_socket()
{
  socket_.async_connect(endpoint_, [this](error_code ec)
  {
    if(!ec)
    {
      cout << "conectado a " << this->socket_.remote_endpoint().address().to_string()
          << ":" << this->socket_.remote_endpoint().port() << '\n';
      escribir_socket("mike;ftw;");
      Sleep(200);
      escribir_socket("ofrecer pser");

      leer_socket();
    }

    else
    {
      cout << "Error conectando: " << ec.value() <<  ": " <<  ec.message() << '\n';
      if(ec.value() == 10056) /*Error 10056: Se solicitó conexión en socket ya conectado*/
      {
        std::error_code ec_cerrar;
        socket_.close(ec_cerrar);
        if(!ec_cerrar)
          conectar_socket();
        else
          std::cout << "Error cerrando y conectando: " << ec_cerrar.value() <<  ": " <<  ec_cerrar.message() << std::endl;
      }
        //...
    }
  });
} //conectar

void fwd::leer_socket()
{
  socket_.async_read_some(asio::buffer(rx_buf_socket_),
    [this](std::error_code ec, std::size_t bytes_leidos)
  {
    if(!ec)
    {
      cout.write(rx_buf_socket_.data(), bytes_leidos);

      /** retransmitir al puerto */
      escribir_puerto_serial(string(rx_buf_socket_.data()));

      memset(rx_buf_socket_.data(), '\0', rx_buf_socket_.size() );
      leer_socket();
    }
    else
    {
      cout << "Error leyendo socket->" << ec.value() <<  ": " << ec.message() << std::endl;
    }
  });
}

void fwd::escribir_socket(std::string str)
{
  tx_buf_socket_ = str;
  asio::async_write(socket_, asio::buffer(tx_buf_socket_.data(), tx_buf_socket_.size()),
    [this](std::error_code ec, std::size_t len)
  {
    if (!ec)
    {
      //cout << "Se escribio " << tx_buf_ << " con " << len << " bytes" << endl;
      /*procesar escritura exitosa*/
    }
  });
}

int main()
{
  try
  {
    asio::io_service servicio;
    //fwd ftw(servicio, "127.0.0.1", "3214", "COM5", 9600);
    fwd ftw(servicio, "201.139.98.214", "3214", "COM5", 9600);
    ftw.iniciar();

    //dbg aver(servicio, "201.139.98.214", "3214");
    //aver.iniciar();

    servicio.run();
  }
  catch (std::exception const& e)
  {
    cout << e.what();
  }
    return 0;
}

//g++.exe -Wall -fexceptions -Wextra -std=c++14 -g -DASIO_STANDALONE -D_WINNT_WIN32=0x0501 -Iinclude -IC:\include -c C:\Proyectos\nube\main.cpp -o obj\Debug\main.o
//g++.exe -Wall -fexceptions -g -DASIO_STANDALONE -D_WINNT_WIN32=0x0501 -IC:\include -c C:\Proyectos\persistence\main.cpp -o obj\Debug\main.o

