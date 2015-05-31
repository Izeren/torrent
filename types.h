#ifndef _TYPES_
#define _TYPES_

typedef std::string Hash_t;

typedef std::string Port_t;

typedef std::string IP_t;

typedef std::map<Hash_t, std::pair<int, std::string> > ClientDataBase_t;

typedef std::pair<Port_t, IP_t> Addr_t;

typedef std::map<Hash_t, std::vector<Addr_t> > DataBase_t;

#endif //_TYPES_