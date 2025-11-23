//
// Created by Matrix on 2025/11/22.
//

#ifndef LAB2QRCODE_CONVERT_H
#define LAB2QRCODE_CONVERT_H

#include <variant>
#include <vector>

#include <QImage>
#include <QString>
#include <QFileInfo>
#include <QByteArray>
#include <ZXing/BitMatrix.h>
#include <ZXing/ImageView.h>
#include <ZXing/MultiFormatWriter.h>
#include <ZXing/ReadBarcode.h>

namespace convert{
    struct result_data_entry {
        using variant_t = std::variant<std::monostate, QImage, QByteArray, std::string>;

        //Empty, QRCode, decoded text, error
        QString source_file_name;
        variant_t data;

        [[nodiscard]] result_data_entry() = default;

        [[nodiscard]] result_data_entry(const QString& source_file_name, const variant_t& data) :
            source_file_name(source_file_name), data(data) {}

        [[nodiscard]] QString get_default_target_name() const {
            if (std::holds_alternative<QImage>(data)) {
                if (!source_file_name.isEmpty())
                    return QFileInfo(source_file_name).baseName() + ".png";
                return "qrcode.png";
            }
            if (std::holds_alternative<QByteArray>(data)) {

                if (!source_file_name.isEmpty())
                    return QFileInfo(source_file_name).completeBaseName() + ".rfa";
                return "decoded.rfa";
            }

            return {};
        }

        template <typename Str, typename T>
            requires (std::constructible_from<variant_t, T&&> && std::constructible_from<QString, Str&&>)
        [[nodiscard]] explicit(!std::convertible_to<T&&, variant_t>)
            result_data_entry(Str&& str_source, T&& dat) noexcept(std::is_nothrow_constructible_v<QString, Str&&> && std::is_nothrow_constructible_v<variant_t, T&&>) :
            source_file_name(std::forward<Str>(str_source)),
            data(std::forward<T>(dat))
        {}

        template <typename T>
            requires (std::constructible_from<std::string, T&&>)
        void set_error(T&& err) noexcept(std::is_nothrow_constructible_v<std::string, T&&>){
            data.emplace<std::string>(std::forward<T>(err));
        }

        void set_error(const QString& err){
            data.emplace<std::string>(err.toStdString());
        }

        explicit operator bool() const noexcept {
            return !std::holds_alternative<std::monostate>(data) && !std::holds_alternative<std::string>(data);;
        }
    };

	struct QRcode_create_config{
		int target_width = 300;
		int target_height = 300;
		ZXing::BarcodeFormat format = ZXing::BarcodeFormat::QRCode;
		int margin = 1;
	};

	[[nodiscard]] inline QImage byte_to_QRCode_qimage(const std::string& text, const QRcode_create_config qrcode_config){
		ZXing::MultiFormatWriter writer(qrcode_config.format);
		writer.setMargin(qrcode_config.margin);

		const auto bitMatrix = writer.encode(text, qrcode_config.target_width, qrcode_config.target_height);
		const auto width = bitMatrix.width();
		const auto height = bitMatrix.height();

		QImage image(width, height, QImage::Format_Grayscale8);

		for (int y = 0; y < height; ++y) {
			uchar* line = image.scanLine(y);
			for (int x = 0; x < width; ++x) {
				line[x] = bitMatrix.get(x, y) ? 0x00 : std::numeric_limits<uchar>::max();
			}
		}

		return image;
	}

    struct result_i2t { //image to text result, 傻瓜式expected
	    enum errcode{
	        success,
	        empty_img,
	        invalid_qrcode
	    };
	    std::string text{};
	    errcode err{};

        [[nodiscard]] explicit(false) result_i2t(const std::string& text) : text(text) {}

        [[nodiscard]] explicit(false) result_i2t(std::string&& text) : text(std::move(text)) {}

        [[nodiscard]] explicit(false) result_i2t(errcode err) : err(err) {}

        explicit operator bool() const noexcept
	    {
	        return !err;
	    }
	};

	[[nodiscard]] inline result_i2t QRcode_to_byte(const std::string& bytes){
	    cv::Mat img = cv::imread(bytes, cv::IMREAD_COLOR);
	    if (img.empty()) {
	        return result_i2t::empty_img;
	    }

	    cv::Mat grayImg;
	    cv::cvtColor(img, grayImg, cv::COLOR_BGR2GRAY);

	    const ZXing::ImageView imageView(grayImg.data, grayImg.cols, grayImg.rows, ZXing::ImageFormat::Lum);
	    const auto result = ZXing::ReadBarcode(imageView);

	    if (!result.isValid()) {
	        return result_i2t::invalid_qrcode;
	    }

	    return result.text();
	}

}

#endif //LAB2QRCODE_CONVERT_H