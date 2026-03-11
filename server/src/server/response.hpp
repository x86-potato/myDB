#pragma once
#include <string>
#include <vector>
#include <cstdint>

// --- Wire protocol enums ---

enum class StatusCode : uint8_t {
    OK,          // simple success (INSERT, UPDATE, DELETE, etc.)
    ERROR,       // error response
    METADATA,    // SELECT column metadata
    ROW,         // SELECT row data
};

enum class PacketFlag : uint8_t {
    LAST_PACKET  = 0,
    MORE_PACKETS = 1,
};

enum class ColumnType : uint8_t {
    INT,
    TEXT,
    FLOAT,
    BOOL,
};

// --- Universal packet envelope ---
// Every response is exactly this structure on the wire:
//   1 byte  status_code
//   1 byte  flag
//   4 bytes payload_length
//   N bytes payload

struct Packet {
    StatusCode status;
    PacketFlag flag;
    uint32_t   payload_length;
    std::vector<uint8_t> payload;
    
};

// --- Metadata payload (status = METADATA, flag = LAST_PACKET) ---
// Wire layout of payload:
//   4 bytes table_count
//   for each table:
//     4 bytes table_name_length
//     N bytes table_name
//     4 bytes column_count
//     for each column:
//       4 bytes column_name_length
//       N bytes column_name
//       1 byte  column_type

struct ColumnDescriptor {
    std::string name;
    ColumnType  type;
};

struct TableDescriptor {
    std::string                   name;
    std::vector<ColumnDescriptor> columns;
};

struct MetadataPayload {
    std::vector<TableDescriptor> tables;
};

// --- Row payload (status = ROW, flag = MORE_PACKETS | LAST_PACKET) ---
// Wire layout of payload:
//   4 bytes column_count
//   for each column:
//     4 bytes column_data_length
//     N bytes column_data

struct RowPayload {
    std::vector<std::string> values;
};

// --- Error payload (status = ERROR, flag = LAST_PACKET) ---
// Wire layout of payload:
//   N bytes error_message (payload_length bytes)

struct ErrorPayload {
    std::string message;
};