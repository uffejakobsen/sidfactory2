#include "runtime/editor/converters/jch/converter_jch.h"
#include "runtime/editor/dialog/dialog_message.h"
#include "runtime/editor/components_manager.h"
#include "runtime/editor/driver/driver_utils.h"
#include "runtime/editor/datasources/datasource_orderlist.h"
#include "runtime/editor/datasources/datasource_sequence.h"
#include "runtime/editor/screens/screen_edit_utils.h"
#include "runtime/emulation/cpumemory.h"
#include "utils/utilities.h"
#include "utils/c64file.h"
#include "libraries/ghc/fs_std.h"
#include "foundation/platform/iplatform.h"
#include <assert.h>

using namespace fs;

namespace Editor
{
	ConverterJCH::ConverterJCH()
		: m_CPUMemory(nullptr)
	{
	}


	ConverterJCH::~ConverterJCH()
	{
	}


	bool ConverterJCH::Convert
	(
		void* inData,
		unsigned int inDataSize,
		Foundation::IPlatform* inPlatform,
		ComponentsManager& inComponentsManager,
		std::function<void(std::shared_ptr<Utility::C64File>)> inSuccessAction
	)
	{
		assert(m_CPUMemory == nullptr);

		// Assert that there's is some data in the first place and the platfomr exists
		assert(inData != nullptr);
		assert(inDataSize > 0);
		assert(inPlatform != nullptr);

		// Detect if the converter can presumably convert the data
		const bool can_convert_this = IsFileValid(inData, inDataSize);

		if (can_convert_this)
		{
			// Store the success action
			m_SuccessAction = inSuccessAction;

			// Create c64 file from the input data
			m_InputData = Utility::C64File::CreateFromPRGData(inData, inDataSize);

			// Read the driver
			if (LoadDestinationDriver(inPlatform))
			{
				// Gather info about the input data
				GatherInputInfo();

				// Import all tables
				ImportTables();

				// Create cpu memory
				m_CPUMemory = new Emulation::CPUMemory(0x10000, inPlatform);
				m_CPUMemory->Lock();
				m_CPUMemory->Clear();
				m_CPUMemory->SetData(m_OutputData->GetTopAddress(), m_OutputData->GetData(), m_OutputData->GetDataSize());
				m_CPUMemory->Unlock();

				// Import order list
				unsigned int max_sequence_index = ImportOrderLists();

				// Import sequences
				ImportSequences(max_sequence_index);

				// Reflect to output
				ReflectToOutput();

				// Destroy cpu memory
				m_CPUMemory->Unlock();
				m_CPUMemory = nullptr;

				// Show a dialog, to test the flow!
				inComponentsManager.StartDialog(std::make_shared<DialogMessage>("JCH Converter", "You will now be converting this file to a JCH thing", 80, true, [&]()
					{
						// Notify the success!
						m_SuccessAction(m_OutputData);
					}
				));
			}

			// Return true, to indicate that the convertion has finished consumed the input
			return true;
		}

		// Return false, if the converter cannot convert the type of data parsed in!
		return false;
	}


	bool ConverterJCH::LoadDestinationDriver(Foundation::IPlatform* inPlatform)
	{
		const path driver_path = inPlatform->Storage_GetDriversHomePath();
		const path driver_path_and_filename = driver_path / "sf2driver_np20_00.prg";

		const int max_file_size = 0x10000;

		// Read test music data to cpu memory
		void* data = nullptr;
		long data_size = 0;

		// Create driver info
		m_DriverInfo = std::make_shared<DriverInfo>();

		if (Utility::ReadFile(driver_path_and_filename.string().c_str(), max_file_size, &data, data_size))
		{
			// Try to parse the data immediately
			m_OutputData = Utility::C64File::CreateFromPRGData(data, static_cast<unsigned int>(data_size));

			// The data has been copied to the c64file container
			delete[] data;

			if (m_OutputData != nullptr)
				m_DriverInfo->Parse(*m_OutputData);

			return m_DriverInfo->IsValid();
		}

		return false;
	}


	void ConverterJCH::GatherInputInfo()
	{
		assert(m_InputData != nullptr);
		assert(m_InputData->GetTopAddress() == 0x0f00);

		const unsigned short address_pointer_wave_table = 0x0fbc;
		const unsigned short address_pointer_filter_table = 0x0fc0;
		const unsigned short address_pointer_pulse_table = 0x0fc2;
		const unsigned short address_pointer_instrument_table = 0x0fc4;
		const unsigned short address_pointer_command_table = 0x0fd0;
		const unsigned short address_pointer_orderlist_v1 = 0x0fc6;
		const unsigned short address_pointer_orderlist_v2 = 0x0fc8;
		const unsigned short address_pointer_orderlist_v3 = 0x0fca;
		const unsigned short address_pointer_sequence_vector_low = 0x0fcc;
		const unsigned short address_pointer_sequence_vector_high = 0x0fce;

		m_InputInfo.m_WaveTableAddress = m_InputData->GetWord(address_pointer_wave_table);
		m_InputInfo.m_FilterTableAddress = m_InputData->GetWord(address_pointer_filter_table);
		m_InputInfo.m_PulseTableAddress = m_InputData->GetWord(address_pointer_pulse_table);
		m_InputInfo.m_InstrumentTableAddress = m_InputData->GetWord(address_pointer_instrument_table);
		m_InputInfo.m_CommandTableAddress = m_InputData->GetWord(address_pointer_command_table);
		m_InputInfo.m_OrderlistV1Address = m_InputData->GetWord(address_pointer_orderlist_v1);
		m_InputInfo.m_OrderlistV2Address = m_InputData->GetWord(address_pointer_orderlist_v2);
		m_InputInfo.m_OrderlistV3Address = m_InputData->GetWord(address_pointer_orderlist_v3);
		m_InputInfo.m_SequenceVectorLowAddress = m_InputData->GetWord(address_pointer_sequence_vector_low);
		m_InputInfo.m_SequenceVectorHighAddress = m_InputData->GetWord(address_pointer_sequence_vector_high);
	}


	bool ConverterJCH::ImportTables()
	{
		const std::vector<DriverInfo::TableDefinition>& table_definitions = m_DriverInfo->GetTableDefinitions();

		auto find_table = [&](const std::string& inName) -> const DriverInfo::TableDefinition*
		{
			for (const DriverInfo::TableDefinition& table_definition : table_definitions)
			{
				if (table_definition.m_Name == inName)
					return &table_definition;
			}

			return nullptr;
		};

		{
			const DriverInfo::TableDefinition* table = find_table("Instruments");
			if (table == nullptr)
				return false;
			CopyTableRowToColumnMajor(m_InputInfo.m_InstrumentTableAddress, table->m_Address, table->m_RowCount, table->m_ColumnCount);
		}

		{
			const DriverInfo::TableDefinition* table = find_table("Commands");
			if (table == nullptr)
				return false;
			CopyTableRowToColumnMajor(m_InputInfo.m_CommandTableAddress, table->m_Address, table->m_RowCount, table->m_ColumnCount);
		}

		{
			const DriverInfo::TableDefinition* table = find_table("Wave");
			if (table == nullptr)
				return false;
			CopyTable(m_InputInfo.m_WaveTableAddress, table->m_Address, table->m_RowCount * table->m_ColumnCount);
		}

		{
			const DriverInfo::TableDefinition* table = find_table("Pulse");
			if (table == nullptr)
				return false;
			CopyTable(m_InputInfo.m_PulseTableAddress, table->m_Address, table->m_RowCount * table->m_ColumnCount);
		}

		{
			const DriverInfo::TableDefinition* table = find_table("Filter");
			if (table == nullptr)
				return false;
			CopyTable(m_InputInfo.m_FilterTableAddress, table->m_Address, table->m_RowCount * table->m_ColumnCount);
		}

		return true;
	}


	unsigned int ConverterJCH::ImportOrderLists()
	{
		std::vector<std::shared_ptr<DataSourceOrderList>> orderlist_data_sources;

		// Create data containers for each track
		ScreenEditUtils::PrepareOrderListsDataSources(*m_DriverInfo, *m_CPUMemory, orderlist_data_sources);

		const unsigned short orderlist_vectors[3] =
		{
			m_InputInfo.m_OrderlistV1Address,
			m_InputInfo.m_OrderlistV2Address,
			m_InputInfo.m_OrderlistV3Address
		};

		const unsigned short orderlist_max_length = orderlist_vectors[1] - orderlist_vectors[0];
		unsigned int max_sequence_index = 0;

		for (int i = 0; i < 3; ++i)
		{
			const unsigned short read_address = orderlist_vectors[i];
			int event_pos = 0;

			for (int offset = 0; offset < orderlist_max_length; offset += 2)
			{
				const unsigned char transpose = m_InputData->GetByte(read_address + offset);
				const unsigned char sequence_index = m_InputData->GetByte(read_address + offset + 1);

				auto& entry = (*orderlist_data_sources[i])[event_pos];

				if (transpose == 0xff)
				{
					entry.m_Transposition = 0xff;
					entry.m_SequenceIndex = 0x00;

					orderlist_data_sources[i]->ComputeLength();

					break;
				}

				entry.m_Transposition = 0x20 + transpose;
				entry.m_SequenceIndex = sequence_index;

				if (sequence_index > max_sequence_index)
					max_sequence_index = sequence_index;

				event_pos++;
			}

			auto packed_data = orderlist_data_sources[i]->Pack();
			orderlist_data_sources[i]->SendPackedDataToBuffer(packed_data);
			m_CPUMemory->Lock();
			orderlist_data_sources[i]->PushDataToSource();
			m_CPUMemory->Unlock();
		}

		return max_sequence_index;
	}


	void ConverterJCH::ImportSequences(unsigned int inMaxSequenceIndex)
	{
		Editor::DriverState driver_state;
		std::vector<std::shared_ptr<DataSourceSequence>> sequence_data_sources;

		// Create data containers for each sequence
		ScreenEditUtils::PrepareSequenceDataSources(*m_DriverInfo, driver_state, *m_CPUMemory, sequence_data_sources);

		for (unsigned int i = 0; i <= inMaxSequenceIndex; ++i)
		{
			unsigned short read_address = (static_cast<unsigned short>(m_InputData->GetByte(m_InputInfo.m_SequenceVectorHighAddress + i)) << 8) | m_InputData->GetByte(m_InputInfo.m_SequenceVectorLowAddress + i);

			ImportSequence(read_address + 2, sequence_data_sources[i]);
		}
	}


	void ConverterJCH::ImportSequence(unsigned short inReadAddress, std::shared_ptr<DataSourceSequence>& inWriteDataSource)
	{
		unsigned int event_pos = 0;

		for (unsigned short i = 0; i < 0x100; i += 2)
		{
			unsigned char command = m_InputData->GetByte(inReadAddress + i);
			unsigned char note = m_InputData->GetByte(inReadAddress + i + 1);

			auto& event = (*inWriteDataSource)[event_pos];

			if (command == 0x7f)
				break;

			if (command >= 0xc0)
			{
				event.m_Command = command;
				event.m_Instrument = 0x80;
			}
			else
			{
				event.m_Command = 0x80;
				event.m_Instrument = command;
			}

			event.m_Note = note;

			event_pos++;
		}

		inWriteDataSource->SetLength(event_pos);

		auto packed_data = inWriteDataSource->Pack();
		inWriteDataSource->SendPackedDataToBuffer(packed_data);
		m_CPUMemory->Lock();
		inWriteDataSource->PushDataToSource();
		m_CPUMemory->Unlock();

	}


	bool ConverterJCH::ReflectToOutput()
	{
		m_CPUMemory->Lock();
		const unsigned short top_of_file_address = m_DriverInfo->GetTopAddress();
		const unsigned short end_of_file_address = DriverUtils::GetEndOfMusicDataAddress(*m_DriverInfo, reinterpret_cast<const Emulation::IMemoryRandomReadAccess&>(*m_CPUMemory));
		const unsigned short data_size = end_of_file_address - top_of_file_address;

		unsigned char* data = new unsigned char[data_size];
		m_CPUMemory->GetData(top_of_file_address, data, data_size);
		m_CPUMemory->Unlock();
		m_OutputData = Utility::C64File::CreateFromData(top_of_file_address, data, data_size);
		delete[] data;

		return true;
	}



	void ConverterJCH::CopyTable(unsigned short inSourceAddress, unsigned short inDestinationAddress, unsigned short inSize)
	{
		for (unsigned short i = 0; i < inSize; ++i)
			(*m_OutputData)[inDestinationAddress + i] = (*m_InputData)[inSourceAddress + i];
	}


	void ConverterJCH::CopyTableRowToColumnMajor(unsigned short inSourceAddress, unsigned short inDestinationAddress, unsigned short inRowCount, unsigned short inColumnCount)
	{
		for (unsigned short r = 0; r < inRowCount; ++r)
		{
			for (unsigned short c = 0; c < inColumnCount; ++c)
			{
				unsigned short src_address = inSourceAddress + c + r * inColumnCount;
				unsigned short dest_address = inDestinationAddress + c * inRowCount + r;
				(*m_OutputData)[dest_address] = (*m_InputData)[src_address];
			}

		}
	}


	void ConverterJCH::CopyWaveTable(unsigned short inSourceAddress, unsigned short inDestinationAddress, unsigned short inSize)
	{
		// This is a bit special, because things are being swapped!
		for (unsigned short i = 0; i < inSize; ++i)
		{
			const unsigned char value = (*m_InputData)[inSourceAddress + i];

			if (value == 0x7f || value == 0x7e)
			{
				(*m_OutputData)[inDestinationAddress + i] = value;
				(*m_OutputData)[inDestinationAddress + inSize  + i] = (*m_InputData)[inSourceAddress + inSize + i];
			}
			else
			{
				(*m_OutputData)[inDestinationAddress + inSize + i] = value;
				(*m_OutputData)[inDestinationAddress + i] = (*m_InputData)[inSourceAddress + inSize + i];
			}
		}
	}


	bool ConverterJCH::IsFileValid(void* inData, unsigned int inDataSize) const
	{
		const unsigned short address_version = 0x0fee;

		if (inDataSize < 0x10000)
		{
			const unsigned short destination_address = Utility::C64File::ReadTargetAddressFromData(inData, inDataSize);

			if (destination_address == 0x0f00)
			{
				auto file = Utility::C64File::CreateFromPRGData(inData, inDataSize);

				unsigned char version_1 = file->GetByte(address_version + 0);
				unsigned char version_2 = file->GetByte(address_version + 1);
				unsigned char version_3 = file->GetByte(address_version + 2);
				unsigned char version_4 = file->GetByte(address_version + 3);
				unsigned char version_5 = file->GetByte(address_version + 4);

				if (version_1 != '2')
					return false;
				if (version_2 != '0')
					return false;
				if (version_3 != '.')
					return false;
				if (version_4 != 'G')
					return false;
				//if (version_4 != '4')		// Not sure that this matters!
				//	return false;

				return true;
			}
		}

		return false;
	}
}