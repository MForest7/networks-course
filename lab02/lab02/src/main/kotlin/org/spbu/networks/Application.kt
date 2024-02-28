package org.spbu.networks

import io.ktor.http.*
import io.ktor.serialization.kotlinx.json.*
import io.ktor.server.application.*
import io.ktor.server.plugins.contentnegotiation.*
import io.ktor.server.request.*
import io.ktor.server.response.*
import io.ktor.server.routing.*
import io.ktor.util.collections.*
import io.ktor.util.pipeline.*
import io.ktor.utils.io.streams.*
import kotlinx.serialization.json.Json
import kotlinx.serialization.json.jsonObject
import java.io.File
import java.util.concurrent.atomic.AtomicInteger

fun main(args: Array<String>) {
    io.ktor.server.netty.EngineMain.main(args)
}

fun Application.module() {
    install(ContentNegotiation) {
        json()
    }

    val nextId = AtomicInteger(0)
    val products = ConcurrentMap<Int, Product>()

    File("icons").mkdir()

    fun PipelineContext<Unit, ApplicationCall>.findProduct(id: Int) = products[id]?.let(::Ok)
        ?: Err { notFound("Not found product with id=$id") }

    routing {
        post("/product") {
            val product = call.receive<AddQuery>().run {
                Product(nextId.getAndIncrement(), name, description) }
            products[product.id] = product
            ok(product)
        }
        get("/product/{product_id}") {
            val id = call.parameters["product_id"]
            id?.toIntOrNull()?.let(products::get)?.let { call.respond(it) }
                ?: call.respond(HttpStatusCode.NotFound, "No product with id=$id")
        }
        put("/product/{product_id}") {
            intParameter("product_id").flatMap(this::findProduct).unwrap { product ->
                val jsonBody = Json.parseToJsonElement(call.receiveText()).jsonObject
                val updatedProduct = updateFromJson(product, jsonBody)
                products[product.id] = updatedProduct
                ok(updatedProduct)
            }
        }
        delete("/product/{product_id}") {
            intParameter("product_id").flatMap(this::findProduct).unwrap { product ->
                products.remove(product.id)
                product.icon?.let(::File)?.takeIf(File::exists)?.delete()
                ok(product)
            }
        }
        get("/products") {
            ok(products.values)
        }
        post("/product/{product_id}/image") {
            intParameter("product_id").flatMap(this::findProduct)
                .unwrap { product ->
                    val imgFile = File("icons/${product.id}.png")
                    call.receiveChannel().readRemaining().inputStream().copyTo(imgFile.outputStream())
                    product.icon = imgFile.path
                    ok("Loaded icon to ${product.icon}")
                }
        }
        get("/product/{product_id}/image") {
            intParameter("product_id").flatMap(this::findProduct)
                .flatMap { icon?.let(::File)?.takeIf(File::exists)
                    .catchNull { badRequest("No icon set for product id=$id") } }
                .unwrap { icon ->
                    call.respondBytes(icon.inputStream().readAllBytes(), ContentType.Image.PNG, HttpStatusCode.OK)
                }
        }
    }
}
