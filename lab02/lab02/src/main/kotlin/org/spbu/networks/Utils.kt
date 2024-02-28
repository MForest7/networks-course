package org.spbu.networks

import io.ktor.http.*
import io.ktor.server.application.*
import io.ktor.server.response.*
import io.ktor.util.pipeline.*
import kotlinx.serialization.json.*

typealias RequestContext = PipelineContext<Unit, ApplicationCall>

inline fun <reified T> updateFromJson(value: T, json: JsonObject): T {
    val merged = Json.encodeToJsonElement(value).jsonObject.plus(json)
    return Json.decodeFromJsonElement<T>(JsonObject(merged))
}

suspend inline fun <reified T : Any> RequestContext.ok(content: T) = call.respond(HttpStatusCode.OK, content)
suspend inline fun <reified T : Any> RequestContext.badRequest(content: T) = call.respond(HttpStatusCode.BadRequest, content)
suspend inline fun <reified T : Any> RequestContext.notFound(content: T) = call.respond(HttpStatusCode.NotFound, content)


sealed interface Result<T> {
    suspend fun <R> flatMap(action: suspend T.() -> Result<R>): Result<R>
    suspend fun unwrap(action: suspend (T) -> Unit)
}

data class Ok<T>(val value: T) : Result<T> {
    override suspend fun <R> flatMap(action: suspend (T) -> Result<R>) = action(value)
    override suspend fun unwrap(action: suspend (T) -> Unit) = action(value)
}
class Err<T>(private val catchError: suspend () -> Unit) : Result<T> {
    override suspend fun <R> flatMap(action: suspend (T) -> Result<R>): Result<R> = Err(catchError)
    override suspend fun unwrap(action: suspend (T) -> Unit) = catchError()
}

fun <T> T?.catchNull(catcher: suspend () -> Unit): Result<T> = this?.let(::Ok) ?: Err(catcher)

suspend fun RequestContext.intParameter(name: String) = call.parameters[name]
    .catchNull {
        badRequest("Not found value for parameter $name") }
    .flatMap { toIntOrNull().catchNull {
        badRequest("Expected integer value of \"$name\", got \"$this\"") } }