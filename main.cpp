#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <ctime>
#include <regex>
#include <cctype>
using namespace std;

// Struttura per rappresentare un libro
struct Libro {
    string titolo;
    string autore;
    int anno;
    int quantita;
};

// Struttura per rappresentare una libreria
struct Libreria {
    string nome;
    vector<Libro> libri;
};

// Struttura per rappresentare un utente
struct Utente {
    string nome;
    string codiceFiscale;
    string telefono;
    bool registrato;
    float sconto;
    double penalita;
};

// Struttura per rappresentare un prestito
struct Prestito {
    string codiceFiscaleUtente;
    string nomeLibreria;
    string titoloLibro;
    time_t dataPrestito;
    time_t dataScadenza;
    bool restituito;
};

// Database globale
vector<Libreria> biblioteca;
vector<Utente> utenti;
vector<Prestito> prestiti;
const float SCONTO_REGISTRATI = 10.0; // 10% di sconto
const double MULTA_GIORNALIERA = 0.50; // 0.50€ al giorno di ritardo

// Funzione per validare il codice fiscale
bool validaCodiceFiscale(const string &cf) {
    if (cf.length() != 16) 
        return false;
    
    regex pattern("^[A-Z]{6}[0-9]{2}[A-Z][0-9]{2}[A-Z][0-9]{3}[A-Z]$");
    return regex_match(cf, pattern);
}

// Funzione per registrare un utente
void registraUtente(const Utente &u, const string &fileCsv) {
    ofstream file(fileCsv, ios::app);
    if (!file.is_open()) {
        cerr << "Errore nell'apertura del file CSV" << endl;
        return;
    }
    
    time_t now = time(nullptr);
    tm *ltm = localtime(&now);
    char data[11];
    strftime(data, sizeof(data), "%d/%m/%Y", ltm);
    
    file << u.nome << "," << u.codiceFiscale << "," << u.telefono << ","
         << data << "," << u.sconto << "\n";
    file.close();
}

// Funzione per calcolare la multa
double calcolaMulta(int giorniRitardo) {
    return giorniRitardo * MULTA_GIORNALIERA;
}

// Funzione per prestare un libro
bool prestaLibro(Utente &u, Libreria &L, const string &titolo, const tm &dataScadenza) {
    for (Libro &libro : L.libri) {
        if (libro.titolo == titolo && libro.quantita > 0) {
            libro.quantita--;
            
            Prestito p;
            p.codiceFiscaleUtente = u.codiceFiscale;
            p.nomeLibreria = L.nome;
            p.titoloLibro = titolo;
            p.dataPrestito = time(nullptr);
            p.dataScadenza = mktime(const_cast<tm*>(&dataScadenza));
            p.restituito = false;
            
            prestiti.push_back(p);
            return true;
        }
    }
    return false;
}

// Funzione per restituire un libro
void restituisciLibro(Utente &u, Libreria &L, const string &titolo, const tm &dataRestituzione) {
    time_t dataRest = mktime(const_cast<tm*>(&dataRestituzione));
    
    for (Prestito &p : prestiti) {
        if (p.codiceFiscaleUtente == u.codiceFiscale && 
            p.nomeLibreria == L.nome && 
            p.titoloLibro == titolo && 
            !p.restituito) {
            
            p.restituito = true;
            
            // Trova il libro e incrementa la quantità
            for (Libro &libro : L.libri) {
                if (libro.titolo == titolo) {
                    libro.quantita++;
                    break;
                }
            }
            
            // Calcola giorni di ritardo
            time_t scadenza = p.dataScadenza;
            if (u.registrato) {
                scadenza += 5 * 24 * 60 * 60; // 5 giorni di tolleranza
            }
            
            if (dataRest > scadenza) {
                int giorniRitardo = (dataRest - scadenza) / (24 * 60 * 60);
                u.penalita += calcolaMulta(giorniRitardo);
                cout << "Multa applicata: " << calcolaMulta(giorniRitardo) << "€" << endl;
            }
            break;
        }
    }
}

// Funzione per segnalare un libro perso
void segnalaLibroPerso(Utente &u, Libreria &L, const string &titolo) {
    for (Prestito &p : prestiti) {
        if (p.codiceFiscaleUtente == u.codiceFiscale && 
            p.nomeLibreria == L.nome && 
            p.titoloLibro == titolo && 
            !p.restituito) {
            
            p.restituito = true;
            u.penalita += 20.0; // Penalità fissa per perdita
            break;
        }
    }
}

// Funzione per visualizzare la biblioteca
void mostraBiblioteca(const vector<Libreria> &biblioteca) {
    cout << left << setw(20) << "Libreria" 
         << setw(30) << "Titolo" 
         << setw(25) << "Autore" 
         << setw(10) << "Anno" 
         << setw(15) << "Disponibilità" << endl;
    cout << string(90, '-') << endl;
    
    for (const Libreria &l : biblioteca) {
        for (const Libro &libro : l.libri) {
            cout << left << setw(20) << l.nome 
                 << setw(30) << libro.titolo 
                 << setw(25) << libro.autore 
                 << setw(10) << libro.anno 
                 << setw(15) << libro.quantita << endl;
        }
    }
}

// Funzione per caricare le librerie da file
void caricaLibrerie(const string &filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Errore nell'apertura del file librerie" << endl;
        return;
    }
    
    biblioteca.clear();
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        
        if (line[0] == '[' && line[line.size()-1] == ']') {
            Libreria lib;
            lib.nome = line.substr(1, line.size()-2);
            biblioteca.push_back(lib);
        } else {
            Libro libro;
            size_t pos1 = line.find(',');
            size_t pos2 = line.find(',', pos1+1);
            size_t pos3 = line.find(',', pos2+1);
            
            if (pos1 != string::npos && pos2 != string::npos && pos3 != string::npos) {
                libro.titolo = line.substr(0, pos1);
                libro.autore = line.substr(pos1+1, pos2-pos1-1);
                libro.anno = stoi(line.substr(pos2+1, pos3-pos2-1));
                libro.quantita = stoi(line.substr(pos3+1));
                
                if (!biblioteca.empty()) {
                    biblioteca.back().libri.push_back(libro);
                }
            }
        }
    }
    file.close();
}

// Funzione per salvare le librerie su file
void salvaLibrerie(const string &filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Errore nella creazione del file librerie" << endl;
        return;
    }
    
    for (const Libreria &l : biblioteca) {
        file << "[" << l.nome << "]" << endl;
        for (const Libro &libro : l.libri) {
            file << libro.titolo << ","
                 << libro.autore << ","
                 << libro.anno << ","
                 << libro.quantita << endl;
        }
        file << endl;
    }
    file.close();
}

// Funzione per caricare gli utenti da CSV
void caricaUtenti(const string &filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Errore nell'apertura del file utenti" << endl;
        return;
    }
    
    utenti.clear();
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        
        Utente u;
        size_t pos1 = line.find(',');
        size_t pos2 = line.find(',', pos1+1);
        size_t pos3 = line.find(',', pos2+1);
        size_t pos4 = line.find(',', pos3+1);
        
        if (pos1 != string::npos && pos2 != string::npos && 
            pos3 != string::npos && pos4 != string::npos) {
            u.nome = line.substr(0, pos1);
            u.codiceFiscale = line.substr(pos1+1, pos2-pos1-1);
            u.telefono = line.substr(pos2+1, pos3-pos2-1);
            u.registrato = true;
            u.sconto = stof(line.substr(pos4+1));
            u.penalita = 0.0;
            
            utenti.push_back(u);
        }
    }
    file.close();
}

// Funzione di ricerca libri
vector<Libro> ricerca(const string &query) {
    vector<Libro> risultati;
    for (const Libreria &l : biblioteca) {
        for (const Libro &libro : l.libri) {
            if (libro.titolo.find(query) != string::npos || 
                libro.autore.find(query) != string::npos) {
                risultati.push_back(libro);
            }
        }
    }
    return risultati;
}

// Funzione per aggiungere un libro
void aggiungiLibro(Libreria &l, const Libro &libro) {
    l.libri.push_back(libro);
}

// Funzione per rimuovere un libro
void rimuoviLibro(Libreria &l, const string &titolo) {
    for (auto it = l.libri.begin(); it != l.libri.end(); ++it) {
        if (it->titolo == titolo) {
            l.libri.erase(it);
            break;
        }
    }
}

// Funzioni di utilità per l'interfaccia
tm parseData(const string &data) {
    tm t = {};
    strptime(data.c_str(), "%d/%m/%Y", &t);
    return t;
}

void stampaMenu() {
    cout << "\n=== MENU PRINCIPALE ===" << endl;
    cout << "1. Visualizza biblioteca" << endl;
    cout << "2. Registra utente" << endl;
    cout << "3. Effettua prestito" << endl;
    cout << "4. Restituisci libro" << endl;
    cout << "5. Segnala libro perso" << endl;
    cout << "6. Ricerca libri" << endl;
    cout << "7. Gestisci libreria" << endl;
    cout << "8. Salva dati" << endl;
    cout << "0. Esci" << endl;
    cout << "Scelta: ";
}

void stampaGestioneLibreria() {
    cout << "\n=== GESTIONE LIBRERIA ===" << endl;
    cout << "1. Aggiungi libro" << endl;
    cout << "2. Rimuovi libro" << endl;
    cout << "3. Torna al menu principale" << endl;
    cout << "Scelta: ";
}

int main() {
    // Caricamento dati iniziali
    caricaLibrerie("librerie.txt");
    caricaUtenti("utenti.csv");
    
    int scelta;
    int libreriaAttiva = 0;
    
    do {
        stampaMenu();
        cin >> scelta;
        cin.ignore();
        
        switch(scelta) {
            case 1: // Visualizza biblioteca
                mostraBiblioteca(biblioteca);
                break;
                
            case 2: { // Registra utente
                Utente u;
                cout << "Nome: ";
                getline(cin, u.nome);
                
                do {
                    cout << "Codice Fiscale (16 caratteri): ";
                    getline(cin, u.codiceFiscale);
                    // Converti in maiuscolo
                    for (char &c : u.codiceFiscale) c = toupper(c);
                } while (!validaCodiceFiscale(u.codiceFiscale));
                
                cout << "Telefono: ";
                getline(cin, u.telefono);
                
                u.registrato = true;
                u.sconto = SCONTO_REGISTRATI;
                u.penalita = 0.0;
                
                utenti.push_back(u);
                registraUtente(u, "utenti.csv");
                cout << "Utente registrato con successo!" << endl;
                break;
            }
                
            case 3: { // Effettua prestito
                if (utenti.empty()) {
                    cout << "Nessun utente registrato!" << endl;
                    break;
                }
                
                cout << "Utenti disponibili:" << endl;
                for (size_t i = 0; i < utenti.size(); i++) {
                    cout << i+1 << ". " << utenti[i].nome << endl;
                }
                
                int sceltaUtente;
                cout << "Seleziona utente: ";
                cin >> sceltaUtente;
                cin.ignore();
                
                if (sceltaUtente < 1 || sceltaUtente > static_cast<int>(utenti.size())) {
                    cout << "Scelta non valida!" << endl;
                    break;
                }
                
                Utente &u = utenti[sceltaUtente-1];
                string titolo, data;
                
                cout << "Titolo libro: ";
                getline(cin, titolo);
                
                cout << "Data scadenza (GG/MM/AAAA): ";
                getline(cin, data);
                tm dataScadenza = parseData(data);
                
                if (prestaLibro(u, biblioteca[libreriaAttiva], titolo, dataScadenza)) {
                    cout << "Prestito effettuato con successo!" << endl;
                } else {
                    cout << "Errore nel prestito. Libro non disponibile." << endl;
                }
                break;
            }
                
            case 4: { // Restituisci libro
                if (utenti.empty()) {
                    cout << "Nessun utente registrato!" << endl;
                    break;
                }
                
                cout << "Utenti disponibili:" << endl;
                for (size_t i = 0; i < utenti.size(); i++) {
                    cout << i+1 << ". " << utenti[i].nome << endl;
                }
                
                int sceltaUtente;
                cout << "Seleziona utente: ";
                cin >> sceltaUtente;
                cin.ignore();
                
                if (sceltaUtente < 1 || sceltaUtente > static_cast<int>(utenti.size())) {
                    cout << "Scelta non valida!" << endl;
                    break;
                }
                
                Utente &u = utenti[sceltaUtente-1];
                string titolo, data;
                
                cout << "Titolo libro: ";
                getline(cin, titolo);
                
                cout << "Data restituzione (GG/MM/AAAA): ";
                getline(cin, data);
                tm dataRest = parseData(data);
                
                restituisciLibro(u, biblioteca[libreriaAttiva], titolo, dataRest);
                cout << "Libro restituito!" << endl;
                break;
            }
                
            case 5: { // Segnala libro perso
                if (utenti.empty()) {
                    cout << "Nessun utente registrato!" << endl;
                    break;
                }
                
                cout << "Utenti disponibili:" << endl;
                for (size_t i = 0; i < utenti.size(); i++) {
                    cout << i+1 << ". " << utenti[i].nome << endl;
                }
                
                int sceltaUtente;
                cout << "Seleziona utente: ";
                cin >> sceltaUtente;
                cin.ignore();
                
                if (sceltaUtente < 1 || sceltaUtente > static_cast<int>(utenti.size())) {
                    cout << "Scelta non valida!" << endl;
                    break;
                }
                
                Utente &u = utenti[sceltaUtente-1];
                string titolo;
                
                cout << "Titolo libro: ";
                getline(cin, titolo);
                
                segnalaLibroPerso(u, biblioteca[libreriaAttiva], titolo);
                cout << "Libro segnalato come perso. Penalita applicata: 20€" << endl;
                break;
            }
                
            case 6: { // Ricerca libri
                string query;
                cout << "Inserisci titolo o autore: ";
                getline(cin, query);
                
                vector<Libro> risultati = ricerca(query);
                
                if (risultati.empty()) {
                    cout << "Nessun risultato trovato." << endl;
                } else {
                    cout << "\nRisultati ricerca:" << endl;
                    cout << left << setw(30) << "Titolo" 
                         << setw(25) << "Autore" 
                         << setw(10) << "Anno" 
                         << setw(15) << "Disponibilita" << endl;
                    cout << string(80, '-') << endl;
                    
                    for (const Libro &libro : risultati) {
                        cout << left << setw(30) << libro.titolo 
                             << setw(25) << libro.autore 
                             << setw(10) << libro.anno 
                             << setw(15) << libro.quantita << endl;
                    }
                }
                break;
            }
                
            case 7: { // Gestione libreria
                int sceltaGestione;
                do {
                    stampaGestioneLibreria();
                    cin >> sceltaGestione;
                    cin.ignore();
                    
                    switch(sceltaGestione) {
                        case 1: { // Aggiungi libro
                            Libro libro;
                            cout << "Titolo: ";
                            getline(cin, libro.titolo);
                            cout << "Autore: ";
                            getline(cin, libro.autore);
                            cout << "Anno: ";
                            cin >> libro.anno;
                            cout << "Quantita: ";
                            cin >> libro.quantita;
                            cin.ignore();
                            
                            aggiungiLibro(biblioteca[libreriaAttiva], libro);
                            cout << "Libro aggiunto con successo!" << endl;
                            break;
                        }
                            
                        case 2: { // Rimuovi libro
                            string titolo;
                            cout << "Titolo libro da rimuovere: ";
                            getline(cin, titolo);
                            rimuoviLibro(biblioteca[libreriaAttiva], titolo);
                            cout << "Libro rimosso!" << endl;
                            break;
                        }
                    }
                } while (sceltaGestione != 3);
                break;
            }
                
            case 8: // Salva dati
                salvaLibrerie("librerie.txt");
                cout << "Dati salvati con successo!" << endl;
                break;
        }
    } while (scelta != 0);
    
    return 0;
}