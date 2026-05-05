
#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"

namespace SpatialIndex
{
	namespace RTree
	{
		class RTree;
		class Leaf;
		class Index;
		class Node;

		typedef Tools::PoolPointer<Node> NodePtr;
		class Node : public SpatialIndex::INode
		{
		public:
			~Node() override;
			Tools::IObject* clone() override;

			uint32_t getByteArraySize() override;
			void loadFromByteArray(const uint8_t* data) override;
			void storeToByteArray(uint8_t** data, uint32_t& len) override;


			id_type getIdentifier() const override;
			void getShape(IShape** out) const override;


			uint32_t getChildrenCount() const override;
			id_type getChildIdentifier(uint32_t index)  const override;
			void getChildShape(uint32_t index, IShape** out)  const override;
                        void getChildData(uint32_t index, uint32_t& length, uint8_t** data) const override;
			uint32_t getLevel() const override;
			bool isIndex() const override;
			bool isLeaf() const override;

		private:
			Node();
			Node(RTree* pTree, id_type id, uint32_t level, uint32_t capacity);

			virtual Node& operator=(const Node&);

			virtual void insertEntry(uint32_t dataLength, uint8_t* pData, Region& mbr, id_type id);
			virtual void deleteEntry(uint32_t index);

			virtual bool insertData(uint32_t dataLength, uint8_t* pData, Region& mbr, id_type id, std::stack<id_type>& pathBuffer, uint8_t* overflowTable);
			virtual void reinsertData(uint32_t dataLength, uint8_t* pData, Region& mbr, id_type id, std::vector<uint32_t>& reinsert, std::vector<uint32_t>& keep);

			virtual void rtreeSplit(uint32_t dataLength, uint8_t* pData, Region& mbr, id_type id, std::vector<uint32_t>& group1, std::vector<uint32_t>& group2);
			virtual void rstarSplit(uint32_t dataLength, uint8_t* pData, Region& mbr, id_type id, std::vector<uint32_t>& group1, std::vector<uint32_t>& group2);

			virtual void pickSeeds(uint32_t& index1, uint32_t& index2);

			virtual void condenseTree(std::stack<NodePtr>& toReinsert, std::stack<id_type>& pathBuffer, NodePtr& ptrThis);

			virtual NodePtr chooseSubtree(const Region& mbr, uint32_t level, std::stack<id_type>& pathBuffer) = 0;
			virtual NodePtr findLeaf(const Region& mbr, id_type id, std::stack<id_type>& pathBuffer) = 0;

			virtual void split(uint32_t dataLength, uint8_t* pData, Region& mbr, id_type id, NodePtr& left, NodePtr& right) = 0;

			RTree* m_pTree{nullptr};


			uint32_t m_level{0};


			id_type m_identifier{-1};
			uint32_t m_children{0};

			uint32_t m_capacity{0};

			Region m_nodeMBR;

			uint8_t** m_pData{nullptr};

			RegionPtr* m_ptrMBR{nullptr};

			id_type* m_pIdentifier{nullptr};

			uint32_t* m_pDataLength{nullptr};

			uint32_t m_totalDataLength{0};

			class RstarSplitEntry
			{
			public:
				Region* m_pRegion;
				uint32_t m_index;
				uint32_t m_sortDim;

				RstarSplitEntry(Region* pr, uint32_t index, uint32_t dimension) :
					m_pRegion(pr), m_index(index), m_sortDim(dimension) {}

				static int compareLow(const void* pv1, const void* pv2)
				{
                    RstarSplitEntry* pe1 = * (RstarSplitEntry**) pv1;
					RstarSplitEntry* pe2 = * (RstarSplitEntry**) pv2;

					assert(pe1->m_sortDim == pe2->m_sortDim);

					if (pe1->m_pRegion->m_pLow[pe1->m_sortDim] < pe2->m_pRegion->m_pLow[pe2->m_sortDim]) return -1;
					if (pe1->m_pRegion->m_pLow[pe1->m_sortDim] > pe2->m_pRegion->m_pLow[pe2->m_sortDim]) return 1;
					return 0;
				}
				static int compareHigh(const void* pv1, const void* pv2)
				{
					RstarSplitEntry* pe1 = * (RstarSplitEntry**) pv1;
					RstarSplitEntry* pe2 = * (RstarSplitEntry**) pv2;
					assert(pe1->m_sortDim == pe2->m_sortDim);
					if (pe1->m_pRegion->m_pHigh[pe1->m_sortDim] < pe2->m_pRegion->m_pHigh[pe2->m_sortDim]) return -1;
					if (pe1->m_pRegion->m_pHigh[pe1->m_sortDim] > pe2->m_pRegion->m_pHigh[pe2->m_sortDim]) return 1;
					return 0;
				}
			};
			class ReinsertEntry
			{
			public:
				uint32_t m_index;
				double m_dist;

				ReinsertEntry(uint32_t index, double dist) : m_index(index), m_dist(dist) {}

				static int compareReinsertEntry(const void* pv1, const void* pv2)
				{
					ReinsertEntry* pe1 = * (ReinsertEntry**) pv1;
					ReinsertEntry* pe2 = * (ReinsertEntry**) pv2;

					if (pe1->m_dist < pe2->m_dist) return -1;
					if (pe1->m_dist > pe2->m_dist) return 1;
					return 0;
				}
			}; // ReinsertEntry

			friend class RTree;
			friend class Leaf;
			friend class Index;
			friend class Tools::PointerPool<Node>;
			friend class BulkLoader;
		}; // Node
	}
}
#pragma GCC diagnostic pop
