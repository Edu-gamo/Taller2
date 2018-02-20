#include <SFML\Graphics.hpp>
#include <SFML\Network.hpp>
#include <string>
#include <cstring>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>

#define MAX_MENSAJES 25
#define MAX_MENSAJES_LENGTH 100

sf::IpAddress ip = sf::IpAddress::getLocalAddress();
sf::TcpSocket socket;
sf::Socket::Status status;

char buffer[2000];
std::size_t received;
std::size_t sent;
std::string old_text = "Connected to: ";

std::vector<std::string> aMensajes;

std::mutex mut;

std::string user_name;

void receiveText(char* text) {
	std::lock_guard<std::mutex> guard(mut);
	aMensajes.push_back(text);
	if (aMensajes.size() > 25)
	{
		aMensajes.erase(aMensajes.begin(), aMensajes.begin() + 1);
	}
}

void thread_function() {
	do {
		status = socket.receive(buffer, MAX_MENSAJES_LENGTH, received);
		receiveText(buffer);
		memset(buffer, 0, sizeof(buffer));
	} while (status == sf::Socket::Done);
	if(status == sf::Socket::Disconnected) receiveText("Se ha perdido la conexion con el servidor");
}

int main() {

	std::cout << "Nombre de usuario: ";
	std::cin >> user_name;

	status = socket.connect(ip, 5000, sf::seconds(5.f));

	if (status == sf::Socket::Done) {
		std::cout << "Conectado al Servidor " << ip << "\n";
		std::string text = user_name + " se ha conectado";
		socket.send(text.c_str(), text.size());
	}
	else {
		std::cout << "Fallo al Conectar con el Servidor " << ip << "\n";
		system("pause");
		exit(0);
	}

	//*************************************************************************//

	sf::Vector2i screenDimensions(800, 600);

	sf::RenderWindow window;
	window.create(sf::VideoMode(screenDimensions.x, screenDimensions.y), "Chat");

	sf::Font font;
	if (!font.loadFromFile("comicSans.ttf")) {
		std::cout << "Can't load the font file" << std::endl;
	}

	sf::String mensaje = ">";

	sf::Text chattingText(mensaje, font, 14);
	chattingText.setFillColor(sf::Color(0, 160, 0));
	chattingText.setStyle(sf::Text::Bold);


	sf::Text text(mensaje, font, 14);
	text.setFillColor(sf::Color(0, 160, 0));
	text.setStyle(sf::Text::Bold);
	text.setPosition(0, 560);

	sf::RectangleShape separator(sf::Vector2f(800, 5));
	separator.setFillColor(sf::Color(200, 200, 200, 255));
	separator.setPosition(0, 550);

	std::thread t(&thread_function); //Thread start

	while (window.isOpen()) {

		sf::Event evento;
		while (window.pollEvent(evento))
		{
			switch (evento.type)
			{
			case sf::Event::Closed:
				window.close();
				break;
			case sf::Event::KeyPressed:
				if (evento.key.code == sf::Keyboard::Escape)
					window.close();
				else if (evento.key.code == sf::Keyboard::Return) {

					//SEND
					mensaje.erase(0, 1);
					if (mensaje == "exit") {
						window.close();
						continue;
					}
					mensaje = user_name + ": " + mensaje;

					socket.send(mensaje.toAnsiString().c_str(), mensaje.getSize());

					//SEND END

					mensaje = ">";
				}
				break;
			case sf::Event::TextEntered:
				if (evento.text.unicode >= 32 && evento.text.unicode <= 126)
					mensaje += (char)evento.text.unicode;
				else if (evento.text.unicode == 8 && mensaje.getSize() > 1)
					mensaje.erase(mensaje.getSize() - 1, mensaje.getSize());
				break;
			}
		}
		window.draw(separator);
		for (size_t i = 0; i < aMensajes.size(); i++)
		{
			std::string chatting = aMensajes[i];
			chattingText.setPosition(sf::Vector2f(0, 20 * i));
			chattingText.setString(chatting);
			window.draw(chattingText);
		}
		std::string mensaje_ = mensaje + "_";
		text.setString(mensaje_);

		window.draw(text);


		window.display();
		window.clear();
	}
	std::string te = user_name + " se ha desconectado";
	socket.send(te.c_str(), te.size());

	t.join(); //Thread end

	socket.disconnect();
	return 0;
}