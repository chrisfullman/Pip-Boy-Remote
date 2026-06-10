import Ajv from 'ajv/dist/2020'
import addFormats from 'ajv-formats'

// JSON Schema files live in schema/ at the repo root, one level above frontend/.
import heartbeatSchema from '../../schema/heartbeat.schema.json'
import stateUpdateSchema from '../../schema/state-update.schema.json'
import inventoryUpdateSchema from '../../schema/inventory-update.schema.json'
import actionResponseSchema from '../../schema/action-response.schema.json'
import mapMarkersUpdateSchema from '../../schema/map-markers-update.schema.json'
import questUpdateSchema from '../../schema/quest-update.schema.json'

const ajv = new Ajv({ strict: false })
addFormats(ajv)

// Compile validators once at module load — reused for every incoming message.
export const validateHeartbeat         = ajv.compile(heartbeatSchema)
export const validateStateUpdate       = ajv.compile(stateUpdateSchema)
export const validateInventoryUpdate   = ajv.compile(inventoryUpdateSchema)
export const validateActionResponse    = ajv.compile(actionResponseSchema)
export const validateMapMarkersUpdate  = ajv.compile(mapMarkersUpdateSchema)
export const validateQuestUpdate       = ajv.compile(questUpdateSchema)
