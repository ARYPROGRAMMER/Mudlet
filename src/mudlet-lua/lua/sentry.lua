----------------------------------------------------------------------------------------
-- Mudlet Sentry Integration
-- Copyright (C) 2024 by ARYPROGRAMMER
-- Copyright (C) 2024 by Stephen Lyons - slysven@virginmedia.com
----------------------------------------------------------------------------------------

--- Initialize crash reporting with Sentry
-- @param enabled boolean flag to enable/disable crash reporting
function initializeSentry(enabled)
    enabled = enabled == nil and true or enabled
    return mudlet.initializeSentry(enabled)
end

--- Set user consent for crash reporting
-- @param enabled boolean flag
function setCrashReportingEnabled(enabled)
    enabled = enabled == nil and true or enabled
    return mudlet.setCrashReportingEnabled(enabled)
end

--- Get current crash reporting status
-- @return boolean indicating if crash reporting is enabled
function isCrashReportingEnabled()
    return mudlet.isCrashReportingEnabled()
end

--- Send a custom message to Sentry
-- @param message string message to capture
-- @param level string optional level (debug, info, warning, error)
function captureMessage(message, level)
    assert(type(message) == "string", "message must be a string")
    level = level or "info"
    assert(type(level) == "string", "level must be a string")
    
    -- Validate level
    local validLevels = {
        debug = true,
        info = true,
        warning = true,
        error = true
    }
    
    if not validLevels[level:lower()] then
        level = "info"
    end
    
    return mudlet.sentryCaptureMessage(message, level)
end

--- Capture an error with additional context
-- @param error string error message
-- @param info table optional additional information
function captureError(error, info)
    assert(type(error) == "string", "error must be a string")
    
    if info then
        assert(type(info) == "table", "info must be a table")
    end
    
    local contextInfo = ""
    if info then
        local infoStrings = {}
        for k, v in pairs(info) do
            table.insert(infoStrings, tostring(k) .. ": " .. tostring(v))
        end
        contextInfo = table.concat(infoStrings, "\n")
    end
    
    return mudlet.sentryCaptureError(error, contextInfo)
end

--- Add debug information to current session
-- @param category string category of the breadcrumb
-- @param message string message to add
function addBreadcrumb(category, message)
    assert(type(category) == "string", "category must be a string")
    assert(type(message) == "string", "message must be a string")
    
    return mudlet.sentryAddBreadcrumb(category, message)
end

--- Set custom context data for crash reports
-- @param name string context name
-- @param data table context data
function setContext(name, data)
    assert(type(name) == "string", "name must be a string")
    assert(type(data) == "table", "data must be a table")
    
    local jsonData = {}
    for k, v in pairs(data) do
        jsonData[tostring(k)] = tostring(v)
    end
    
    return mudlet.sentrySetContext(name, jsonData)
end

--- Clear custom context data
-- @param name string context name to clear
function clearContext(name)
    assert(type(name) == "string", "name must be a string")
    return mudlet.sentryClearContext(name)
end