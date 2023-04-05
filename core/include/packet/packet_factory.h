#include "include/network/aopacket.h"
#include <memory>

class PacketFactory
{
  public:
    static std::shared_ptr<AOPacket> createPacket(QString header, QStringList contents);
    static std::shared_ptr<AOPacket> createPacket(QString raw_packet);
    template <typename T>
    static void registerClass(QString header) { class_map[header] = &createInstance<T>; };

  private:
    template <typename T>
    static std::shared_ptr<AOPacket> createInstance(QStringList contents) { return std::make_shared<T>(contents); };
    template <typename T>
    static std::shared_ptr<AOPacket> createInstance(QString header, QStringList contents) { return std::make_shared<T>(header, contents); };
    typedef std::map<QString, std::shared_ptr<AOPacket> (*)(QStringList)> type_map;

    static inline type_map class_map;
};
